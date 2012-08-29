
/*
 * Copyright (C) liu_wei
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
	ngx_str_t	verifyHeader;
} ngx_http_misc_loc_conf_t;

static char * ngx_http_verify_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void * ngx_http_misc_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_variable_first_req_initiated(ngx_http_request_t *r,
													   ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t ngx_http_misc_commands[] = {

	{ ngx_string("verifyHdr_GetVal"),
	  NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
	  ngx_http_verify_header,
	  NGX_HTTP_LOC_CONF_OFFSET,			
	  0,
	  NULL },

	ngx_null_command
};


static ngx_http_module_t  ngx_http_misc_module_ctx = {
	NULL,								   /* preconfiguration */
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


static char * 
ngx_http_verify_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_misc_loc_conf_t *mlcf = conf;
	ngx_str_t  *value;	
	ngx_http_variable_t *v;

	value = cf->args->elts;

	//	TODO:	检查参数的合法性

	mlcf->verifyHeader = value[1];

	if (value[2].data[0] != '$') {	
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
			"invalid variable name \"%V\"", &value[2]);
		return NGX_CONF_ERROR;
	}

	value[2].len--;
	value[2].data++;

	v = ngx_http_add_variable(cf, &value[2], NGX_HTTP_VAR_NOCACHEABLE);		
	if (v == NULL) {
		return NGX_CONF_ERROR;
	}

	v->get_handler = ngx_http_variable_first_req_initiated;

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
	ngx_connection_t			 *c;

	c = r->connection;
	
	conf = ngx_http_get_module_loc_conf(r, ngx_http_misc_module);

	if (c->extendBackup.found) {
		
		v->data = c->extendBackup.var_value.data;
		v->len = c->extendBackup.var_value.len;
		v->valid = 1;
		v->no_cacheable = 1;
		v->not_found = 0;
	
	} else {
	
		if (c->requests == 1) {

			//	连接中第一个请求将被检查是否含有指定头

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

					if (ngx_strncasecmp(header[i].lowcase_key, conf->verifyHeader.data, 
						conf->verifyHeader.len) == 0)
					{
						c->extendBackup.var_value.len = header[i].value.len;
						c->extendBackup.var_value.data = ngx_pnalloc(c->pool, header[i].value.len);

						if (c->extendBackup.var_value.data == NULL) {
							return NGX_ERROR;
						}

						c->extendBackup.found = 1;
						ngx_memcpy(c->extendBackup.var_value.data, header[i].value.data, header[i].value.len);

						break;
					}
				}	//	end for
			}

		}
			
		*v = ngx_http_variable_null_value;
	}	

	return NGX_OK;
}


