
/*
 * Copyright (C) liu_wei
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

//	存放头域的值
typedef struct {
	unsigned            index:8;
	ngx_str_t           var_value;
} ngx_misc_header_value;

//	存放需要检查的KEY
typedef struct {
	unsigned            index:8;	
	ngx_int_t          req_num;			//	请求序号
	ngx_str_t           header;			
} ngx_misc_header_t;

typedef struct {
	unsigned             number:8;	
	ngx_array_t			*verifyHeader;		//	array of ngx_misc_header_t
} ngx_http_misc_loc_conf_t;

static char * ngx_http_verify_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void * ngx_http_misc_create_loc_conf(ngx_conf_t *cf);
static ngx_int_t ngx_http_variable_first_req_initiated(ngx_http_request_t *r,
													   ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t ngx_http_misc_commands[] = {

	{ ngx_string("get_req_header_value"),
	  NGX_HTTP_LOC_CONF|NGX_CONF_TAKE3,
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
	ngx_misc_header_t  *p_header;

	if (mlcf->verifyHeader == NULL) {
	
        mlcf->verifyHeader = ngx_array_create(cf->pool, 2, sizeof(ngx_misc_header_t));
        if (mlcf->verifyHeader == NULL) {
            return NGX_CONF_ERROR;
        }
    }
	
	value = cf->args->elts;

	mlcf->number++;
			
	p_header = ngx_array_push(mlcf->verifyHeader);
    if (p_header == NULL) {
        return NGX_CONF_ERROR;
    }
	
	p_header->header = value[2];
	p_header->index = mlcf->number;	
	
	p_header->req_num = ngx_atoi(value[1].data, value[1].len);
    if (p_header->req_num == NGX_ERROR) {
        return "invalid number";
    }

	if (value[3].data[0] != '$') {	
		ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
			"invalid variable name \"%V\"", &value[3]);
		return NGX_CONF_ERROR;
	}

	value[3].len--;
	value[3].data++;

	v = ngx_http_add_variable(cf, &value[3], NGX_HTTP_VAR_NOCACHEABLE);		
	if (v == NULL) {
		return NGX_CONF_ERROR;
	}

	v->get_handler = ngx_http_variable_first_req_initiated;
	
	v->data = (uintptr_t) p_header;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_variable_first_req_initiated(ngx_http_request_t *r,
									  ngx_http_variable_value_t *v, uintptr_t data)
{
	ngx_list_part_t              *part;
	ngx_table_elt_t              *header;
	ngx_uint_t					 i;
	ngx_connection_t			 *c;
	ngx_misc_header_t  			 *p_header;
	ngx_misc_header_value		 *p_value;

	c = r->connection;
	p_header = (ngx_misc_header_t *) data;

	if (r == r->main
		&& c->requests == (ngx_uint_t) p_header->req_num) 
	{
		//	连接中第一个请求将被检查是否含有指定头
		part = &r->headers_in.headers.part;
		header = part->elts;

		if (p_header->header.len) {
		
			ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0, "search header : %V", &p_header->header);

			for (i = 0; /* void */; i++) {

				if (i >= part->nelts) {
					if (part->next == NULL) {
						break;
					}

					part = part->next;
					header = part->elts;
					i = 0;
				}

				//	在客户端的请求头域中查找指定header
				if (ngx_strncasecmp(header[i].lowcase_key, p_header->header.data, 
					p_header->header.len) == 0)
				{	
					if (c->misc_array == NULL) {
						c->misc_array = ngx_array_create(c->pool, 2, sizeof(ngx_misc_header_value));
						if (c->misc_array == NULL) {
							return NGX_ERROR;
						}
					}
										
					p_value = ngx_array_push(c->misc_array);
					if (p_value == NULL) {
						return NGX_ERROR;
					}
					
					p_value->index = p_header->index;
					
					//	设置头域的value值
					p_value->var_value.len = header[i].value.len;						
					p_value->var_value.data = ngx_pnalloc(c->pool, header[i].value.len);
					if (p_value->var_value.data == NULL) {
						return NGX_ERROR;
					}
				
					ngx_memcpy(p_value->var_value.data, header[i].value.data, header[i].value.len);

					break;
				}
			}	//	end for
		}
	} else {
		
		if (c->misc_array != NULL) {
	
			p_value = c->misc_array->elts;
	
			for (i = 0; i < c->misc_array->nelts; i++) {
				
				if (p_value[i].index == p_header->index) {
				
					v->data = p_value[i].var_value.data;
					v->len = p_value[i].var_value.len;
					v->valid = 1;
					v->no_cacheable = 1;
					v->not_found = 0;
					
					return NGX_OK;
				}
			}		
		} 	
	}
	
	*v = ngx_http_variable_null_value;

	return NGX_OK;
}


