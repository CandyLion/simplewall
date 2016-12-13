#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Acceleration
#define IDA_MAIN 100

// Menu Id
#define IDM_MAIN 100
#define IDM_TRAY 101
#define IDM_LISTVIEW 102
#define IDM_EDITOR 103

// Dialogs
#define IDD_MAIN 100
#define IDD_EDITOR 101
#define IDD_SETTINGS 102
#define IDD_SETTINGS_1 103
#define IDD_SETTINGS_2 104
#define IDD_SETTINGS_3 105
#define IDD_SETTINGS_4 106
#define IDD_SETTINGS_5 107
#define IDD_SETTINGS_6 108

// Main Dlg
#define IDC_LISTVIEW 1000
#define IDC_START_BTN 1001
#define IDC_SETTINGS_BTN 1002
#define IDC_EXIT_BTN 1003
#define IDC_STATUSBAR 1004

// Editor Dlg
#define IDC_NAME 2000
#define IDC_NAME_EDIT 2001
#define IDC_PROTOCOL 2002
#define IDC_PROTOCOL_EDIT 2003
#define IDC_DIRECTION 2004
#define IDC_DIRECTION_EDIT 2005
#define IDC_ACTION 2006
#define IDC_ACTION_EDIT 2007
#define IDC_RULES 2008
#define IDC_RULES_EDIT 2009
#define IDC_RULES_HELP 2010
#define IDC_RULES_LINKS 2011
#define IDC_ENABLED_CHK 2012

// Settings Dlg
#define IDC_NAV 1000
#define IDC_APPLY 1001
#define IDC_CLOSE 1002

#define IDC_TITLE_1 1003
#define IDC_TITLE_2 1004
#define IDC_TITLE_3 1005
#define IDC_TITLE_4 1006
#define IDC_TITLE_5 1007
#define IDC_TITLE_6 1008

#define IDC_ALWAYSONTOP_CHK 100
#define IDC_LOADONSTARTUP_CHK 101
#define IDC_STARTMINIMIZED_CHK 102
#define IDC_SKIPUACWARNING_CHK 103
#define IDC_CHECKUPDATES_CHK 104

#define IDC_LANGUAGE_HINT 105
#define IDC_LANGUAGE 106

#define IDC_USEBLOCKLIST_CHK 107
#define IDC_USEBLOCKLIST_HINT 108
#define IDC_INSTALLBOOTTIMEFILTERS_CHK 109
#define IDC_INSTALLBOOTTIMEFILTERS_HINT 110
#define IDC_USEUPDATECHECKING_CHK 111
#define IDC_USEUPDATECHECKING_HINT 112

#define IDC_DISABLEWINDOWSFIREWALL_CHK 113

#define IDC_CONFIRMEXIT_CHK 114
#define IDC_CONFIRMDELETE_CHK 115
#define IDC_CONFIRMLOGCLEAR_CHK 116

#define IDC_SHOWBALLOONTIPS_CHK 117
#define IDC_NOTIFICATIONSILENT_CHK 118

#define IDC_ENABLELOG_CHK 119
#define IDC_LOGPATH 120
#define IDC_LOGPATH_BTN 121
#define IDC_LOGVIEWER_TITLE 122
#define IDC_LOGVIEWER 123
#define IDC_LOGVIEWER_BTN 124

#define IDC_ENABLENOTIFICATIONS_CHK 125
#define IDC_NOTIFICATIONTIMEOUT_HINT 126
#define IDC_NOTIFICATIONTIMEOUT 127

#define IDC_ENABLEAPPSCOLLECTOR_CHK 128

#define IDC_COLORS 200
#define IDC_COMMON 201
#define IDC_EDITOR 202

// Main Menu
#define IDM_SETTINGS 2000
#define IDM_EXIT 2001
#define IDM_PURGEN 2002
#define IDM_FIND 2003
#define IDM_FINDNEXT 2004
#define IDM_REFRESH 2005
#define IDM_ALWAYSONTOP_CHK 2007
#define IDM_ICONSSMALL 2008
#define IDM_ICONSLARGE 2009
#define IDM_ICONSISHIDDEN 2010
#define IDM_SORTBYFNAME 2011
#define IDM_SORTBYFDIR 2012
#define IDM_SORTISDESCEND 2013
#define IDM_ENABLELOG_CHK 2014
#define IDM_ENABLENOTIFICATIONS_CHK 2015
#define IDM_ENABLEAPPSCOLLECTOR_CHK 2016
#define IDM_LOGSHOW 2017
#define IDM_LOGCLEAR 2018
#define IDM_WEBSITE 2019
#define IDM_DONATE 2020
#define IDM_CHECKUPDATES 2021
#define IDM_ABOUT 2022

// Tray Menu
#define IDM_TRAY_SHOW 3000
#define IDM_TRAY_START 3001
#define IDM_TRAY_MODEWHITELIST 3002
#define IDM_TRAY_MODEBLACKLIST 3003
#define IDM_TRAY_MODETRUSTNOONE 3004
#define IDM_TRAY_ENABLELOG_CHK 3005
#define IDM_TRAY_ENABLENOTIFICATIONS_CHK 3006
#define IDM_TRAY_ENABLEAPPSCOLLECTOR_CHK 3007
#define IDM_TRAY_LOGSHOW 3008
#define IDM_TRAY_LOGCLEAR 3009
#define IDM_TRAY_SETTINGS 3010
#define IDM_TRAY_WEBSITE 3011
#define IDM_TRAY_ABOUT 3012
#define IDM_TRAY_EXIT 3013

// Listview Menu
#define IDM_ADD_FILE 4000
#define IDM_ALL 4001
#define IDM_REFRESH2 4002
#define IDM_EXCLUDENOTIFICATIONS 4003
#define IDM_EXCLUDELOG 4004
#define IDM_EXPLORE 4005
#define IDM_COPY 4006
#define IDM_CHECK 4007
#define IDM_UNCHECK 4008

#define IDM_ADD 4009
#define IDM_EDIT 4010
#define IDM_DELETE 4011

#define IDM_SELECT_ALL 4012

#define IDM_ADD_PROCESS 5000
#define IDM_DEFAULT 6000
#define IDM_RULES_SYSTEM 7000
#define IDM_RULES_SPECIAL 8000

// Strings
#define IDS_UPDATE_NO 1001
#define IDS_UPDATE_YES 1002

#define IDS_FILE 1003
#define IDS_SETTINGS 1004
#define IDS_EXIT 1005
#define IDS_EDIT 1006
#define IDS_PURGEN 1007
#define IDS_FIND 1008
#define IDS_FINDNEXT 1009
#define IDS_REFRESH 1010
#define IDS_VIEW 1011
#define IDS_ICONS 1012
#define IDS_ICONSSMALL 1013
#define IDS_ICONSLARGE 1014
#define IDS_ICONSISHIDDEN 1015
#define IDS_SORT 1016
#define IDS_SORTBYFDIR 1017
#define IDS_SORTBYFNAME 1018
#define IDS_SORTISDESCEND 1019
#define IDS_LANGUAGE 1020
#define IDS_HELP 1021
#define IDS_WEBSITE 1022
#define IDS_DONATE 1023
#define IDS_CHECKUPDATES 1024
#define IDS_ABOUT 1025

#define IDS_TRAY_SHOW 2000
#define IDS_TRAY_START 2001
#define IDS_TRAY_STOP 2002
#define IDS_TRAY_MODE 2003
#define IDS_TRAY_RULES 2004
#define IDS_TRAY_SYSTEM_RULES 2005
#define IDS_TRAY_SPECIAL_RULES 2006
#define IDS_TRAY_LOG 2007

#define IDS_ADD_FILE 3000
#define IDS_ADD_PROCESS 3001
#define IDS_ALL 3002
#define IDS_EXCLUDENOTIFICATIONS 3003
#define IDS_EXCLUDELOG 3004
#define IDS_EXPLORE 3005
#define IDS_COPY 3006
#define IDS_CHECK 3007
#define IDS_UNCHECK 3008

#define IDS_ADD 3009
#define IDS_EDIT2 3010
#define IDS_DELETE 3011

#define IDS_MODE_WHITELIST 4000
#define IDS_MODE_BLACKLIST 4001
#define IDS_MODE_TRUSTNOONE 4002

#define IDS_RULE_DHCP 5000
#define IDS_RULE_DNS 5001
#define IDS_RULE_NETWORKDISCOVERY 5002
#define IDS_RULE_NTP 5003
#define IDS_RULE_SNMP 5004
#define IDS_RULE_SSDP 5005
#define IDS_RULE_WUAUSERV 5006
#define IDS_RULE_OUTBOUND_ICMP 5007
#define IDS_RULE_INBOUND_ICMP 5008
#define IDS_RULE_INBOUND 5009

#define IDS_HIGHLIGHT_SYSTEM 6000
#define IDS_HIGHLIGHT_NETWORK 6001
#define IDS_HIGHLIGHT_INVALID 6002
#define IDS_HIGHLIGHT_SILENT 6003

#define IDS_GROUP_1 7000
#define IDS_GROUP_2 7001

#define IDS_EDITOR 7002

#define IDS_NAME 7003
#define IDS_DIRECTION 7004
#define IDS_ACTION 7005
#define IDS_RULE 7006
#define IDS_DATE 7007
#define IDS_FILEPATH 7008
#define IDS_ADDRESS 7009

#define IDS_DIRECTION_1 7011
#define IDS_DIRECTION_2 7012
#define IDS_DIRECTION_3 7013

#define IDS_ACTION_1 7014
#define IDS_ACTION_2 7015

#define IDS_RULES_LINKS 7016

#define IDS_RULES_HELP 7018

#define IDS_ENABLED_CHK 7019

#define IDS_SETTINGS_1 8000
#define IDS_SETTINGS_2 8001
#define IDS_SETTINGS_3 8002

#define IDS_APPLY 8008
#define IDS_CLOSE 8009

#define IDS_TITLE_1 8010
#define IDS_TITLE_2 8011
#define IDS_TITLE_3 8012
#define IDS_TITLE_4 8013
#define IDS_TITLE_5 8014
#define IDS_TITLE_6 8015

#define IDS_ALWAYSONTOP_CHK 9000
#define IDS_LOADONSTARTUP_CHK 9001
#define IDS_STARTMINIMIZED_CHK 9002
#define IDS_SKIPUACWARNING_CHK 9003
#define IDS_CHECKUPDATES_CHK 9004

#define IDS_LANGUAGE_HINT 9005

#define IDS_USEBLOCKLIST_CHK 9006
#define IDS_USEBLOCKLIST_HINT 9007
#define IDS_INSTALLBOOTTIMEFILTERS_CHK 9008
#define IDS_INSTALLBOOTTIMEFILTERS_HINT 9009
#define IDS_USEUPDATECHECKING_CHK 9010
#define IDS_USEUPDATECHECKING_HINT 9011

#define IDS_DISABLEWINDOWSFIREWALL_CHK 9012
#define IDS_ENABLEWINDOWSFIREWALL_CHK 9013

#define IDS_SHOWBALLOONTIPS_CHK 9014
#define IDS_NOTIFICATIONSILENT_CHK 9015

#define IDS_CONFIRMEXIT_CHK 9016
#define IDS_CONFIRMDELETE_CHK 9017
#define IDS_CONFIRMLOGCLEAR_CHK 9018

#define IDS_ENABLELOG_CHK 9019
#define IDS_LOGSHOW 9020
#define IDS_LOGCLEAR 9021
#define IDS_LOGVIEWER_TITLE 9022

#define IDS_ENABLENOTIFICATIONS_CHK 9023
#define IDS_NOTIFICATIONTIMEOUT_HINT 9024

#define IDS_ENABLEAPPSCOLLECTOR_CHK 9025

#define IDS_ALWAYSPERFORMTHISCHECK_CHK 9026

#define IDS_QUESTION 10000
#define IDS_QUESTION_START 10001
#define IDS_QUESTION_STOP 10002
#define IDS_QUESTION_EXIT 10003

#define IDS_STATUS_TOTAL 10004
#define IDS_STATUS_EMPTY 10005

#define IDS_STATUS_FILTERS_FAILED 10006

#define IDS_STATUS_NOPRIVILEGES 10007

// Icons
#define IDI_MAIN 100
#define IDI_INACTIVE 101

#endif // __RESOURCE_H__
