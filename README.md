nginx_http_misc_module
======================
###实现功能：
    haproxy结合nginx实现获取用户来源IP，haproxy只能针对连接里的第一个请求进行HTTP header处理, 如果想知道用户来源IP只能对第一个请求增加
    X-Forwarded-For头。当haproxy将请求发送给nginx时，nginx中想获取用户来源IP并通过配置方式灵活使用时，将只能在第一个请求中保存用户
    来源IP，那么haproxy与nginx连接中的后续请求将无法获取客户端的来源信息。解决这个问题，可以在haproxy和nginx两端修改，在haproxy端可以通
    过使用http1.0协议对每个请求建立一次连接，这样用户来源IP每次都转发到后端服务器，这种方法的弊端就是浪费大量的资源和时间。在nginx端解
    决，由于nginx中变量的生存周期与请求生存周期相同，那么通过连接中第一个请求之后的请求将无法知道用户来源IP，所以通过提升nginx中变量的
    生存周期可以解决这个问题。

###模块指令：
    directive:  get_req_header_value
    Syntax:     get_req_header_value req_number header $CUSTOM_VAR
    Default:    NONE
    Context:    location
    This directive save the connection number specified in the request header field value and saved in the variables
