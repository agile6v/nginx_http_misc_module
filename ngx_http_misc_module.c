
/*
 * Copyright (C) liu_wei
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
	ngx_str_t	verifyHeader;
	ngx_str_t	headerValue;
	ngx_uint_t  state;
	ngx_uint_t  last_conn_addr;
} ngx_http_misc_loc_conf_t;

static char * ngx_http_verify_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void * ngx_http_misc_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_misc_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_variable_first_req_initiated(ngx_http_request_t *r,
													   ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t ngx_http_misc_commands[] = {

	{ ngx_string("verifyHeader"),
	  NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
	  ngx_http_verify_header,
	  NGX_HTTP_LOC_CONF_OFFSET,			
	  0,
	  NULL },

	ngx_null_command
};


static ngx_http_module_t  ngx_http_misc_module_ctx = {
	ngx_http_misc_add_variables,           /* preconfiguration */
	NULL,                   			   /* postconfiguration */

	NULL,                                  /* create main configuration */
	NULL,                                  /* init main configuration */

	NULL,                                  /* create server configuration */
	NULL,                                  /* merge server configuration */

	ngx_http_misc_create_loc_conf,         /* create location configuration */
	NULL          						   /* merge location configuration */
};

ngx_module_t  ngx_http_misc_module = {
	NGX_MODULE_V1,
	&ngx_http_misc_module_ctx,             /* module context */
	ngx_http_misc_commands,                /* module directives */
	NGX_HTTP_MODULE,                       /* module type */
	NULL,                                  /* init master */
	NULL,                                  /* init module */
	NULL,                                  /* init process */
	NULL,                                  /* init thread */
	NULL,                                  /* exit thread */
	NULL,                                  /* exit process */
	NULL,                                  /* exit master */
	NGX_MODULE_V1_PADDING
};


static ngx_str_t   ngx_http_misc_first_req_initiated = ngx_string("is_first_req_initiated");


static void *
ngx_http_misc_create_loc_conf(ngx_conf_t *cf)
{
	ngx_http_misc_loc_conf_t  *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_misc_loc_conf_t));
	if (conf == NULL) {
		return NULL;
	}

	return conf;
}

static ngx_int_t
ngx_http_misc_add_variables(ngx_conf_t *cf)
{
	ngx_http_variable_t  *var;

	var = ngx_http_add_variable(cf, &ngx_http_misc_first_req_initiated, NGX_HTTP_VAR_NOCACHEABLE);
	if (var == NULL) {
		return NGX_ERROR;
	}

	var->get_handler = ngx_http_variable_first_req_initiated;
    
	return NGX_OK;
}


static char * 
ngx_http_verify_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_misc_loc_conf_t *mlcf = conf;
	ngx_str_t  *value;	

	value = cf->args->elts;

	//	TODO:	检查参数的合法性

	mlcf->verifyHeader = value[1];
	mlcf->headerValue = value[2];


    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_variable_first_req_initiated(ngx_http_request_t *r,
									  ngx_http_variable_value_t *v, uintptr_t data)
{
	ngx_http_misc_loc_conf_t	 *conf;
	ngx_list_part_t              *part;
	ngx_table_elt_t              *header;
	ngx_uint_t					 i;

	enum {
		sw_start = 0,
		sw_connection
	} state;

	conf = ngx_http_get_module_loc_conf(r, ngx_http_misc_module);

	state = conf->state;

	switch(state) {

		case sw_connection:

			if ((ngx_uint_t) r->connection == conf->last_conn_addr  
				 && r->connection->requests > 1) 
			{
				*v = ngx_http_variable_true_value;
				break;

			} else {
				state = sw_start;				//	如果connection地址不相等时，将重新检查host标记
			}

		case sw_start:

			part = &r->headers_in.headers.part;
			header = part->elts;

			if (conf->verifyHeader.len) {

				for (i = 0; /* void */; i++) {

					if (i >= part->nelts) {
						if (part->next == NULL) {
							break;
						}

						part = part->next;
						header = part->elts;
						i = 0;
					}

					if (ngx_strncasecmp(header[i].lowcase_key, conf->verifyHeader.data, conf->verifyHeader.len) == 0 
						&& ngx_strncasecmp(header[i].value.data, conf->headerValue.data, conf->headerValue.len) == 0)
					{
						state = sw_connection;
						conf->last_conn_addr = (ngx_uint_t) r->connection;
						break;
					}
				}
			}



		default:
			ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
				"misc_module internal error [%d]!", state);

			state = sw_start;
	}

	conf->state = state;

	return NGX_OK;
}


