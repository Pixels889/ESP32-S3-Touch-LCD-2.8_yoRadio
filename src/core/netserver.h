#ifndef netserver_h
#define netserver_h
#include "../AsyncWebServer/ESPAsyncWebServer.h"
#define APPEND_GROUP(name) strcat(nsBuf, "\"" name "\",")

enum requestType_e : uint8_t  { PLAYLIST=1, STATION=2, STATIONNAME=3, ITEM=4, TITLE=5, VOLUME=6, NRSSI=7, BITRATE=8, MODE=9, EQUALIZER=10, BALANCE=11, PLAYLISTSAVED=12, STARTUP=13, GETINDEX=14, GETACTIVE=15, GETSYSTEM=16, GETSCREEN=17, GETTIMEZONE=18, GETWEATHER=19, GETCONTROLS=20, DSPON=21, SDPOS=22, SDLEN=23, SDSNUFFLE=24, SDINIT=25, GETPLAYERMODE=26, CHANGEMODE=27 };
enum import_e      : uint8_t  { IMDONE=0, IMPL=1, IMWIFI=2 };
const char emptyfs_html[] PROGMEM = R"(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1, minimum-scale=0.25"><meta charset="UTF-8">
<link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAYFBMVEUAAADYw1PcyVjYxFTaxlXYxFTbx1bcyVjZxVXbyFfcyFfaxlbax1bcyVjcyVjbyFfbyFfZxVXaxlbbx1fcyFjcyVjbx1fZxVXcyFjcyVjax1bbyFfcyVjbyFfax1bWwVKMlHGzAAAAH3RSTlMA+wv0zu6dBeVqSryjMRaCU97Fjz8liNk5HbFdDnWsEHoUsAAAAeFJREFUWMPtlllyrDAMRS1P2NjMQzc9RPvf5Ut1IPYjDRbJR1KVnD8Z7i1ZsgXsh1JW3usrC9Ta+2og620DiCjaaY65U4AIpqLqBb7R3B5xJucYRpI+U7jgHwsVLgjSLu74DmSvMTdhQVMMHAYeBhiQFAO5Y3CiGFzWBhDilmKQ4zsqm5uwQGvkCRfsytFkJIOhWWo+vz8uCfWMRqEVAJwsn+PsKgFA+YJR4UWe50Oc1Gt8vrFfyGC19153+afUvVMA+ADAaH5QXhvA/wB3yEICfgAqsvys8BngiPor4AaSpM8BN7lQRrrAbcBSLvMeKqmvVhtYh8mxqjCi7Tnnk4YDKYzRy9DPA2Uy9CoYDBShsCrKitxCnUUnm7qHFwyUYTlOAXYHWxP0TTzBbm1UBGIPfMkDZRcMur1bFPdAxEQPXhI1TNLSj+HxK9l9u8H41RrcKQZub5THbdxA7M3WAZL/EvRp0PDPGEgM9CxBqo9mYMcpAAPyzNZMx2aysUUWzYSi7lzSwALGGG3rvO/zurajM4BQJh0aXAGglACYg2v6uw64h2ZJfOIcp2lxh4ZgkEncRjAKF8AtYCI53M2mQc1IlNrAM7lyZ0akHKURsVaokxuLYxfD6ot8w+nOFuyP5/wDsZKME0E1GogAAAAASUVORK5CYII=">
<title>ёRadio - 网页端上传工具</title><style>html, body { margin: 0; padding: 0; height: 100%; } body{background-color:#000;color:#e3d25f;font-size:20px;display:flex;flex-direction:column;}
hr{margin:20px 0;border:0; border-top:#555 1px solid;} p{text-align:center;margin-bottom:10px;} section{max-width:500px; text-align:center;margin:0 auto 30px auto;padding:20px;flex:1;}
.hidden{display:none;}a { color: var(--accent-color); text-decoration: none; font-weight: bold } a:hover { text-decoration: underline }
#copy { text-align: center; padding: 14px; font-size: 14px; }
input[type=file]{color:#ccc;} input[type=file]::file-selector-button, input[type=submit]{border:2px solid #e3d25f;color:#000;padding:6px 16px;border-radius:25px;background-color:#e3d25f;margin:0 6px;cursor:pointer;}
input[type=submit]{font-size:18px;text-transform:uppercase;padding:8px 26px;margin-top:10px;font-family:Times;} span{color:#ccc} .flex{display:flex;justify-content: space-around;margin-top:10px;}
input[type=text],input[type=password]{width:170px;background:#272727;color:#e3d25f;padding:6px 12px;font-size:20px;border:#2d2d2d 1px solid;margin:4px 0 0 4px;border-radius:4px;outline:none;}
@media screen and (max-width:480px) {section{zoom:0.7;-moz-transform:scale(0.7);}}
</style>
<script type="text/javascript" src="/variables.js"></script>
</head><body>
<section>
<h2>ёRadio - 网页端上传工具</h2>
<hr />
<span>从 <i>yoRadio/data/www/</i> 选择 <u>所有</u> 文件<br />并使用下面的表单上传</span>
<hr />
<form action="/webboard" method="post" enctype="multipart/form-data">
<p><label for="www">www:</label> <input type="file" name="www" id="www" multiple></p>
<hr />
<span>-= 可选 =-<br />您也可以从备份中上传 <i>playlist.csv</i><br />和 <i>wifi.csv</i> 文件</span>
<p><label for="data">wifi:</label><input type="file" name="data" id="data" multiple></p>
<hr />
<p><input type="submit" name="submit" value="上传文件"></p>
</form>
<div style="padding:10px 0 0;" id="wupload">
<hr />
<form name="wifiform" method="post" enctype="multipart/form-data">
<span>-= 可选 =-<br />如果您无法从PC连接到192.168.4.1地址<br />请先设置WiFi连接</span>
<div class="flex"><div><label for="ssid">WiFi名称:</label><input type="text" id="ssid" name="ssid" value="" maxlength="30" autocomplete="off"></div>
<div><label for="pass">密码:</label><input type="password" id="pass" name="pass" value="" maxlength="40" autocomplete="off"></div>
</div>
<p><input type="submit" name="submit" value="保存凭据"></p>
</form>
</div>
</section>
<p><a href="/emergency">紧急固件上传</a></p>
<div id="copy">由 <a target="_blank" href="https://github.com/e2002/yoradio/">ёRadio</a> 提供技术支持<span id="version"></span></div>
</body>
<script>
document.wifiform.action = `/${formAction}`;
if(playMode=='player') document.getElementById("wupload").classList.add("hidden");
document.getElementById("version").innerHTML=` | v${yoVersion}`;
</script>
</html>
)";
const char index_html[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <meta name="theme-color" content="#e3d25f">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="default">
  <link rel="icon" type="image/png" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAYFBMVEUAAADYw1PcyVjYxFTaxlXYxFTbx1bcyVjZxVXbyFfcyFfaxlbax1bcyVjcyVjbyFfbyFfZxVXaxlbbx1fcyFjcyVjbx1fZxVXcyFjcyVjax1bbyFfcyVjbyFfax1bWwVKMlHGzAAAAH3RSTlMA+wv0zu6dBeVqSryjMRaCU97Fjz8liNk5HbFdDnWsEHoUsAAAAeFJREFUWMPtlllyrDAMRS1P2NjMQzc9RPvf5Ut1IPYjDRbJR1KVnD8Z7i1ZsgXsh1JW3usrC9Ta+2og620DiCjaaY65U4AIpqLqBb7R3B5xJucYRpI+U7jgHwsVLgjSLu74DmSvMTdhQVMMHAYeBhiQFAO5Y3CiGFzWBhDilmKQ4zsqm5uwQGvkCRfsytFkJIOhWWo+vz8uCfWMRqEVAJwsn+PsKgFA+YJR4UWe50Oc1Gt8vrFfyGC19153+afUvVMA+ADAaH5QXhvA/wB3yEICfgAqsvys8BngiPor4AaSpM8BN7lQRrrAbcBSLvMeKqmvVhtYh8mxqjCi7Tnnk4YDKYzRy9DPA2Uy9CoYDBShsCrKitxCnUUnm7qHFwyUYTlOAXYHWxP0TTzBbm1UBGIPfMkDZRcMur1bFPdAxEQPXhI1TNLSj+HxK9l9u8H41RrcKQZub5THbdxA7M3WAZL/EvRp0PDPGEgM9CxBqo9mYMcpAAPyzNZMx2aysUUWzYSi7lzSwALGGG3rvO/zurajM4BQJh0aXAGglACYg2v6uw64h2ZJfOIcp2lxh4ZgkEncRjAKF8AtYCI53M2mQc1IlNrAM7lyZ0akHKURsVaokxuLYxfD6ot8w+nOFuyP5/wDsZKME0E1GogAAAAASUVORK5CYII=">
  <link rel="stylesheet" href="theme.css" type="text/css" />
  <link rel="stylesheet" href="style.css" type="text/css" />
  <script type="text/javascript" src="variables.js"></script>
  <script type="text/javascript" src="script.js"></script>
  <script type="text/javascript" src="dragpl.js"></script>
  </head>
<body>
<div id="content" class="hidden progmem">
</div><!--content-->
<div id="progress"><span id="loader"></span></div>
<div id="heap"></div>
</body>
</html>
)";
const char emergency_form[] PROGMEM = R"(
<form method="POST" action="/update" enctype="multipart/form-data">
  <input type="hidden" name="updatetarget" value="fw" />
  <label for="uploadfile">上传固件</label>
  <input type="file" id="uploadfile" accept=".bin,.hex" name="update" />
  <input type="submit" value="更新" />
</form>
)";
struct nsRequestParams_t
{
  requestType_e type;
  uint8_t clientId;
};

class NetServer {
  public:
    import_e importRequest;
    bool resumePlay;
    char chunkedPathBuffer[40];
    char nsBuf[BUFLEN], nsBuf2[BUFLEN];
  public:
    NetServer() {};
    bool begin(bool quiet=false);
    void loop();
    void requestOnChange(requestType_e request, uint8_t clientId);
    void setRSSI(int val) { rssi = val; };
    int  getRSSI()        { return rssi; };
    void chunkedHtmlPage(const String& contentType, AsyncWebServerRequest *request, const char * path);
    void onWsMessage(void *arg, uint8_t *data, size_t len, uint8_t clientId);
    bool irRecordEnable;
#if IR_PIN!=255
    void irToWs(const char* protocol, uint64_t irvalue);
    void irValsToWs(); 
#endif
    void resetQueue();
  private:
    requestType_e request;
    QueueHandle_t nsQueue;
    char _wscmd[65], _wsval[65];
    char wsBuf[BUFLEN*2];
    int rssi;
    uint32_t playerBufMax;
    void getPlaylist(uint8_t clientId);
    bool importPlaylist();
    static size_t chunkedHtmlPageCallback(uint8_t* buffer, size_t maxLen, size_t index);
    void processQueue();
    int _readPlaylistLine(File &file, char * line, size_t size);
};

extern NetServer netserver;
extern AsyncWebSocket websocket;

#endif