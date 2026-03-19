基于微雪ESP32-S3-Touch-LCD-2.8的yoradio。

中文化处理（包括浏览器端）；
为了听感连续，变化不突兀，音量调节做了映射。

触摸控制播放列表（长按左上部分打开电台列表，长按右上部分打开sd卡列表，滑动选择单击后，中间高亮选中项开始播放）；
播放控制（点击播放或暂停，上半部分左滑列表下一项，右滑上一项）；
调节音量（屏幕下部左右滑动）；
调节亮度（屏幕右侧上下滑动）；
3分钟无操作息屏，短按电源键亮屏；
亮屏时短按电源键切换播放模式，长按电源键松手关机。

如需修改注意编译环境设置和开发板版本选择。
----------------------------------------------------------------------------------------------
为了存放更多电台，可以修改spiffs分区大小，如果你不需要就不必更改。
目的：增大spiffs空间，便于存放更多电台的playlist.csv文件（minimal spiffs空间只有190kb）
方法：https://wiki.makerfabs.com/Larger%20flash%20and%20memory.html
<img width="1147" height="943" alt="编译环境" src="https://github.com/user-attachments/assets/aad75236-9d60-406d-9220-be3e80a05e7d" />
<img width="979" height="317" alt="开发板管理器esp32的版本2 0 11" src="https://github.com/user-attachments/assets/1a01c2a7-5c44-4f08-8d63-2e033eb0b64f" />
