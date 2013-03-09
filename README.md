nginx_http_misc_module
======================
实现功能：
---------------------
    haproxy只能针对连接里的第一个请求进行HTTP header处理, 如果想知道用户来源IP只能对第一个请求增加X-Forwarded-For头。当
    haproxy将请求发送给nginx时，nginx中想获取用户来源IP并通过配置方式灵活使用时，将只能在第一个请求中保存用户来源IP，那
    么haproxy与nginx连接中的后续请求将无法获取客户端的来源信息。解决这个问题可以在haproxy和nginx两端修改，在haproxy端可
    以通过使用http1.0协议对每个请求建立一次连接，这样用户来源IP每次都转发到后端服务器，这种方法的弊端就是浪费大量的资源
    和时间。在nginx端解决，由于nginx中变量的生存周期与请求生存周期相同，那么连接中第一个请求以后的请求将无法知道用户来源
    IP，所以在nginx端解决是比较合理的。

    directive:  get_req_header_value
    Syntax:     get_req_header_value req_number header $CUSTOM_VAR
    Default:    NONE
    Context:    location
    This directive save the connection number specified in the request header field value and saved in the variables
