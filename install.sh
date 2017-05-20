#!/bin/sh
#Auto Make Install Nginx FLV MP4 Server
#Define Path
LINUXVERSION=`lsb_release -a | grep "Description" | awk -F ":" '{print $2}'`
 
SRC_DIR=/usr/src/nginx/
NGX_DIR=/usr/local/nginx/
CODE=0

#install development tools
yum groupinstall "development tools" -y

#copy install file

rm -rf $SRC_DIR
mkdir -p $SRC_DIR
chmod 777 $SRC_DIR
cp -fp ./* $SRC_DIR
cd $SRC_DIR ; cd .. ; chmod 777 -R $SRC_DIR/

#install pcre
cd $SRC_DIR ; tar -xzf pcre-8.35.tar.gz ;cd pcre-8.35 && ./configure && make &&make install
if
	[ "$?" != "$CODE" ]; then
	  echo "pcre-8.35 install failed!"
	exit 1
else
	echo "pcre-8.35 install success!"
fi
#install openssl
cd $SRC_DIR ; tar -xzf openssl-1.0.2d.tar.gz ;cd openssl-1.0.2d && ./config && make &&make install
if
	[ "$?" != "$CODE" ]; then
	  echo "openssl-1.0.2d install failed!"
	exit 1
else
	echo "openssl-1.0.2d install success!"
fi
#install zlib-1.2.8.tar.gz
cd $SRC_DIR ; tar -xzf zlib-1.2.8.tar.gz ;cd zlib-1.2.8 && ./configure && make &&make install
if
	[ "$?" != "$CODE" ]; then
	  echo "zlib-1.2.8.tar.gz install failed!"
	exit 1
else
	echo "zlib-1.2.8 install success!"
fi
#tar nginx_mod_h264_streaming-2.2.7.tar.gz
cd $SRC_DIR ; tar -xzf nginx_mod_h264_streaming-2.2.7.tar.gz
cp -fp ngx_http_streaming_module.c nginx_mod_h264_streaming-2.2.7/src
if
	[ "$?" != "$CODE" ]; then
	  echo "ngx_http_streaming_module.c copy failed!"
	exit 1
else
	echo "ngx_http_streaming_module.c copy success!"
fi
#tar nginx_mod_h264_streaming-2.2.7.tar.gz
cd $SRC_DIR ; tar -xzf nginx-accesskey.tar.gz
if
	[ "$?" != "$CODE" ]; then
	  echo "tar -xzf nginx-accesskey.tar.gz failed!"
	exit 1
else
	echo "tar -xzf nginx-accesskey.tar.gz success!"
fi

#unzip nginx-rtmp-module-master.zip
cd $SRC_DIR ; unzip nginx-rtmp-module-master.zip 
if
	[ "$?" != "$CODE" ]; then
	  echo "unzip nginx-rtmp-module-master.zip failed!"
	exit 1
else
	echo "unzip nginx-rtmp-module-master.zip success!"
fi

#Nginx install
cd $SRC_DIR ; tar -xzf  nginx-1.6.3.tar.gz ;cd nginx-1.6.3 && ./configure --with-debug --prefix=/usr/local/nginx --add-module=../nginx_mod_h264_streaming-2.2.7 --add-module=../nginx-rtmp-module-master  --with-openssl=../openssl-1.0.2d --user=root --group=root --with-http_flv_module --with-http_mp4_module --with-http_stub_status_module --with-pcre=../pcre-8.35 --add-module=/usr/src/nginx/nginx-accesskey && make &&make install
if
    [ "$?" == "$CODE" ];then
    echo "The Nginx  Make install Success ! "
else
    echo "The Nginx  Make install Failed ! "
    exit 1;
fi

#Install  Yamdi Tools
cd  $SRC_DIR ; tar -xzf  yamdi-1.9.tar.gz ;cd yamdi-1.9/ &&make &&make install
if
	[ "$?" != "$CODE" ]; then
	  echo "yamdi-1.9.tar.gz install failed!"
	exit 1
else
	echo "yamdi-1.9 install success!"
fi





#copy nginx.conf
cp -fp $SRC_DIR/nginx.conf $NGX_DIR/conf
if
	[ "$?" != "$CODE" ]; then
	  echo "nginx.conf copy failed!"
	exit 1
else
	echo "nginx.conf copy success!"
fi
  

FILEDIR=`cat $NGX_DIR/conf/nginx.conf | grep "root" | awk -F " " '{print $2}'  | sed -n 2p | sed -e 's/;//g'`
HTTPPORT=`cat $NGX_DIR/conf/nginx.conf | grep "listen" | awk -F " " '{print $2}'  | sed -n 2p | sed -e 's/;//g'`
RTMPPORT=`cat $NGX_DIR/conf/nginx.conf | grep "listen" | awk -F " " '{print $2}'  | sed -n 1p | sed -e 's/;//g'`


#Config Nginx Service
if [ ! -d "$FILEDIR" ]; then
     mkdir -p "$FILEDIR"
     echo "mkdir $FILEDIR successful!"
fi

  
#prev condition    
ln -s /usr/local/lib/libpcre.so.1 /lib64
if
	[ "$?" != "$CODE" ]; then
	  echo "ln -s /usr/local/lib/libpcre.so.1 failed!"
else
	echo "ln /usr/local/lib/libpcre.so.l success!"
fi
                                                                          
#start Nginx service
$NGX_DIR/sbin/nginx -t
[ $?  -eq  $CODE ]&& $NGX_DIR/sbin/nginx
[ $? -eq $CODE ]&& $NGX_DIR/sbin/nginx -s reload
IP=`ifconfig eth0|grep "Bcast" |awk -F":" '{print $2}'|cut -d" " -f 1`
echo -e "\033[31m";
echo -e "Please pay attention to the following words!"
echo "Attention:http-port:$HTTPPORT;rtmp-port:$RTMPPORT;File-Dir:$FILEDIR"
echo -e "exit!"
echo -e "\033[0m";

#make nginx service into linux
chmod a+x $SRC_DIR/nginx
cp $SRC_DIR/nginx /etc/init.d/nginx
chmod a+x /etc/init.d/nginx
#centos 
if [ -n "`whereis yum | awk -F ':' '{print $2}'`" ]; then
 if [ `cat /etc/rc.local |grep "/etc/init.d/nginx" | wc -l` -eq 0 ];then
     echo "/etc/init.d/nginx" >> /etc/rc.local
 fi
fi
#reboot
chkconfig --add nginx
chkconfig --level 2345 nginx on
exit 0
