大致的思路如下：
1. 用户全部终结到linux上，由linux做dns/dhcp/转发（可以NAT，也可以不NAT，我们有足够的公网IP，不打算NAT)
2. 利用ipset 做用户IP和mac的检查，创建了2个ipset，其中user是已经登录的用户，newuser是无法自动上线必须通过web登录的用户。
   user的数据包直接通过，剩下的80端口数据包利用NFQUEUE重定向到登录页面。其余数据包如果来自newuser直接丢弃，剩下的利用NFQUEUE交给自动登录（如果之前登录过，会自动登录）。
3. 重定向程序redir_url是CERNET2014文章
4. 为了简化用户使用，用mac作为重要识别号，用户第一次登陆时记录mac和手机号，以后一段时间就能直接用

CentOS 6 需要mysql-devel httpd libnetfilter_queue-devel

注：CentOS 6使用的ipset有bug，增加97个 ip,mac 类型的项目后，执行ipset list会导致kernel panic。
因此建议使用Debian stable, 请参考debian目录下说明

