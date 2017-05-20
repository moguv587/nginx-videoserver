# nginx-videoserver
一键搭建NGINX视频服务器，支持rmtp.mp4等,默认支持Nginx-accesskey防盗链 one key to build NGINX video server, support rmtp.mp4 etc...

## 安装
cd nginx-videoserver</br>
chmod +x install.sh</br>
./install.sh</br>

## 配置文件
NGINX文件位置 /usr/src/nginx </br>
NGINX配置目录 /usr/local/nginx/</br>
视频文件存放目录 /data/contentftp</br>

## 防盗链配置
默认防盗链配置已经写入/usr/local/nginx/conf/nginx.conf 第87到93行，已经中文注释，可自行修改

## 防盗链测试播放方法
        <?
        $ipkey= md5("pass".$_SERVER['REMOTE_ADDR']);
        echo "<a href=file.mp4>file</a><br />";
        echo "<a href=file.mp4?key=".$ipkey.">file.rar?key=".$ipkey.">file</a><br />";
        ?>

## 加密视频播放
播放mp3/mp4的加密文件（以bak/dcf结尾）</br>
存储（12.mp4.bak 12.mp4.dcf）用12.mp4访问 遇到dcf的访问bak文件</br>
存储（12.mp3.bak 12.mp3.dcf）用12.mp3.bak访问</br>
请将nginx.conf中的vod 改成ondemand</br>
