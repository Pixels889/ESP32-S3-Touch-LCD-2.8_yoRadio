// displayL10n_custom.h - 中文翻译
// 此文件会被包含在 namespace LANG 内，因此不要重复写 namespace

// 播放器状态
static const char* const_PlReady   = "就绪";
static const char* const_PlStopped = "已停止";
static const char* const_PlConnect = "连接中...";

// 对话框标题
static const char* const_DlgVolume = "音量";
static const char* const_DlgLost   = "网络断开";
static const char* const_DlgUpdate = "正在升级";
static const char* const_waitForSD = "正在读取SD卡...";
static const char* const_DlgNextion = "请使用Nextion屏幕操作";

// 天气
static const char* const_getWeather = "获取天气中...";

// 启动信息格式
static const char* bootstrFmt       = "正在连接 %s...";

// AP模式提示
static const char* apNameTxt        = "WiFi名称:";
static const char* apPassTxt        = "密码:";
static const char* apSettFmt        = "请访问 http://%s/ 进行配置";

// 数值格式
static const char* numtxtFmt        = "%d";
static const char* iptxtFmt         = "IP: %s";
static const char* rssiFmt          = "%d dBm";
static const char* bitrateFmt       = "%d kbps";

// 天气相关
static const char* weatherUnits   = "metric";          // 单位：公制
static const char* weatherLang    = "zh_cn";           // 语言：简体中文

// 天气显示格式：描述，温度，体感温度，气压，湿度，风速，风向
static const char* weatherFmt     = "%s, 温度 %.1f°C (体感 %.1f°C), 气压 %d hPa, 湿度 %d%%, 风速 %.1f m/s, %s";

// 风向数组（16方位）
static const char* wind[16] = {
    "北风", "东北偏北", "东北", "东北偏东",
    "东风", "东南偏东", "东南", "东南偏南",
    "南风", "西南偏南", "西南", "西南偏西",
    "西风", "西北偏西", "西北", "西北偏北"
};

// 星期名称 (tm_wday: 0=星期日)
static const char* dow[7] = {
    "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
};

// 月份名称 (tm_mon: 0=一月)
static const char* mnths[12] = {
    "一月", "二月", "三月", "四月", "五月", "六月",
    "七月", "八月", "九月", "十月", "十一月", "十二月"
};