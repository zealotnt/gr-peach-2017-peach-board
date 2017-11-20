#include "mbed.h"
#include "TLV320_RBSP.h"
#include "cProcess.hpp"
#include "mStorage.hpp"
#include "mWav.hpp"
#include "define.h"
#include "rtos.h"
#include "grHwSetup.h"
#include "grUtility.h"
#include "Json.h"
#include "cJson.h"
#include "cNodeManager.h"
#include "Websocket.h"
#include <string>
#include "gr-robot/gr-robot-led.hpp"
#include "gr-robot/gr-robot-audio-upstream.hpp"

#define MODULE_PREFIX           "[MAIN] "
#define DBG_INFO(...)           printf(MODULE_PREFIX); printf(__VA_ARGS__)
#define SYS_MAIL_PARAM_NUM      (1)     /* Elements number of mail parameter array */
#define MAIL_QUEUE_SIZE         4

typedef enum {
    SYS_MAILID_DUMMY = 0,
    SYS_MAIL_WS_CLOSE,         /* Notifies main thread websocket closing. */
    SYS_MAILID_NUM
} SYS_MAIL_ID;

typedef struct {
    SYS_MAIL_ID     mail_id;
    uint32_t        param[SYS_MAIL_PARAM_NUM];
} sys_mail_t;
static Mail<sys_mail_t, MAIL_QUEUE_SIZE> sysMailBox;

typedef struct
{
    char valueAction[40];
} serverJsonResp_t;

extern void initPlayerConfig();
extern void grRobot_mp3_player();
void notifyMain_websocketClose(uint32_t reasonId);
static NodeManager *peachDeviceManager;

bool parseActionJson(char* body, serverJsonResp_t *resp)
{
    Json json (body, strlen (body));
    char *keyAction = "action";

    if(!json.isValidJson())
    {
        DBG_INFO("Json invalid!\n");
        return NULL;
    }

    int actionIdx = json.findKeyIndexIn ( keyAction , 0 );
    if ( actionIdx == -1 )
    {
        // Error handling part ...
        DBG_INFO("\"%s\" does not exist ... return!!",keyAction);
        return NULL;
    }
    // Find the first child index of key-node "city"
    int valueIndex = json.findChildIndexOf ( actionIdx, -1 );
    if ( valueIndex > 0 )
    {
        const char * valueStart  = json.tokenAddress ( valueIndex );
        int          valueLength = json.tokenLength ( valueIndex );
        strncpy ( resp->valueAction, valueStart, valueLength );
        resp->valueAction [valueLength] = 0; // NULL-terminate the string
    }

    return true;
}

static bool send_sys_mail(const SYS_MAIL_ID mail_id, const uint32_t param0)
{
    bool            ret = false;
    osStatus        stat;
    sys_mail_t      * const p_mail = sysMailBox.alloc();

    if (p_mail != NULL) {
        p_mail->mail_id = mail_id;
        p_mail->param[0] = param0;
        stat = sysMailBox.put(p_mail);
        if (stat == osOK) {
            ret = true;
        } else {
            (void) sysMailBox.free(p_mail);
        }
    }
    return ret;
}

static bool sysRecvMail(SYS_MAIL_ID * const p_mail_id, uint32_t * const p_param0)
{
    bool            ret = false;
    osEvent         evt;
    sys_mail_t      *p_mail;

    if ((p_mail_id != NULL) && (p_param0 != NULL)) {
        evt = sysMailBox.get(osWaitForever);
        if (evt.status == osEventMail) {
            p_mail = (sys_mail_t *)evt.value.p;
            if (p_mail != NULL) {
                *p_mail_id = p_mail->mail_id;
                *p_param0 = p_mail->param[0];
                ret = true;
            }
            (void) sysMailBox.free(p_mail);
        }
    }
    return ret;
}

void notifyMain_websocketClose(uint32_t reasonId)
{
    send_sys_mail(SYS_MAIL_WS_CLOSE, reasonId);
}

void mainContinueGetAction()
{
    send_sys_mail(SYS_MAIL_WS_CLOSE, 0);
}


static bool do_action(char* action, char* toStr)
{
    printf("Action:%s\n", action);
    char *pt;
    bool found = false;
    if(strcmp(action,"on") == 0)
    {
        DBG_INFO("Do on device!\n");
        pt = strtok (toStr,",");

        while (pt != NULL) {
            DBG_INFO("%s\n", pt);
            string name(pt);
            int id = peachDeviceManager->getNodeIdByName(name);
            printf("Id: %d\n", id);
            if( id >= 0)
            {
                printf("IP: %s\n", peachDeviceManager->getIpDevice(id).c_str());
                peachDeviceManager->NodeRelayOn(peachDeviceManager->getIpDevice(id));
            }
            else
                printf("Dont know device!\n");
            //on off
            pt = strtok (NULL, ",");
        }
    }
    else if (strcmp(action,"off") == 0)
    {
        DBG_INFO("Do off device\n");
        pt = strtok (toStr,",");
        while (pt != NULL) {
            DBG_INFO("%s\n", pt);
            string name(pt);
            int id = peachDeviceManager->getNodeIdByName(name);
            if( id >= 0)
            {
                printf("IP: %s\n", peachDeviceManager->getIpDevice(id).c_str());
                peachDeviceManager->NodeRelayOff(peachDeviceManager->getIpDevice(id));
            }
            else
                printf("Dont know device!\n");
            pt = strtok (NULL, ",");
        }
    }
    else
    {
        DBG_INFO("Dont know action\n");
        return false;
    }

    return true;
}

static bool processActionEndpoint(char* body)
{
    Json json (body, strlen (body));
    char valueAction[10] = "";
    char valueTo[100] = "";
    char keyAction[7]="action";
    char keyTo[3]="to";

    if(!json.isValidJson())
    {
        DBG_INFO("Json invalid!\n");
    }

    int keyIndex = json.findKeyIndexIn ( keyAction , 0 );
    if ( keyIndex == -1 )
    {
        // Error handling part ...
        DBG_INFO("\"%s\" does not exist ... do something!!",keyAction);
        return false;
    }
    // Find the first child index of key-node "city"
    int valueIndex = json.findChildIndexOf ( keyIndex, -1 );
    if ( valueIndex > 0 )
    {
        const char * valueStart  = json.tokenAddress ( valueIndex );
        int          valueLength = json.tokenLength ( valueIndex );
        strncpy ( valueAction, valueStart, valueLength );
        valueAction [ valueLength ] = 0; // NULL-terminate the string
    }

    keyIndex = json.findKeyIndexIn ( keyTo , 0 );
    if ( keyIndex == -1 )
    {
        // Error handling part ...
        DBG_INFO("\"%s\" does not exist ... do something!!",keyTo);
        return false;
    }

    // Find the first child index of key-node "city"
    valueIndex = json.findChildIndexOf ( keyIndex, -1 );
    if ( valueIndex > 0 )
    {
        const char * valueStart  = json.tokenAddress ( valueIndex );
        int          valueLength = json.tokenLength ( valueIndex );
        strncpy ( valueTo, valueStart, valueLength );
        valueTo [ valueLength ] = 0; // NULL-terminate the string

        return do_action(valueAction,valueTo);
    }

    return false;
}


typedef enum
{
    ACTION_IDLE = 0x00,
    ACTION_DONE_WW,
    ACTION_STREAM_CMD,
    ACTION_PREPARE_PLAYING,
    ACTION_PLAY_AUDIO,
    ACTION_GET_ACTION_JSON,
} actionType_t;
const char actionCmdStr[][40] = {
    "idle",
    "done-wake-word",
    "stream-cmd",
    "prepare-playing",
    "play-audio",
    "get-action-json"
    ""
};

#define ENABLE_STREAM_AUDIO()       grRobot_audioEnableRead(); audioStreamTask.signal_set(0x1);
#define DISABLE_STREAM_AUDIO()      audioStreamTask.signal_clr(0x1); grRobot_audioDisableRead();

int main_gr_robot() {
    NetworkInterface* network = grInitEth();
    initPlayerConfig();
    SYS_MAIL_ID mailId;
    uint32_t mailParams[SYS_MAIL_PARAM_NUM];
    bool status1, status2;
    char serverRespStr[1024];
    serverJsonResp_t serverResParsed;

    // init the devices manager
    peachDeviceManager = new NodeManager(network);
    peachDeviceManager->NodeAdd(DEV_1_IP, DEV_1_NAME);
    peachDeviceManager->NodeAdd(DEV_2_IP, DEV_2_NAME);
    if (peachDeviceManager->NodeStatusUpdate(DEV_1_IP, &status1) != 0) {
        printf("Can't update %s status\r\n", DEV_1_NAME);
    }
//    if (peachDeviceManager->NodeStatusUpdate(DEV_2_IP, &status2) != 0) {
//        printf("Can't update %s status\r\n", DEV_2_NAME);
//    }
    peachDeviceManager->PostNodeStatus(ADDRESS_HTTP_SERVER);

    // create the thread
    Thread audioReadTask(grRobot_audio_read_task, NULL, osPriorityNormal, 1024*32);
    Thread audioStreamTask(grRobot_audio_stream_task, NULL, osPriorityNormal, 1024*32);
    Thread ledBlinkTask(grRobot_led_blinker_task, NULL, osPriorityNormal, 512*32);

    while(1) {
        // currently there is only notify ws close, so no need to parse the mail
        // ask the server what to do next
        DBG_INFO(">>>>Before HttpGetStatus>>>>\r\n");
        if (grHttpGet(network, "/", serverRespStr, sizeof(serverRespStr)) == false) {
            mainContinueGetAction();
            DBG_INFO("HTTP Get action fail\r\n");
            Thread::wait(500);
            continue;
        }
        DBG_INFO("<<<<HttpGetStatusSuccess<<<<\r\n");
        parseActionJson(serverRespStr, &serverResParsed);
        for (int i = 0; i < ARRAY_LEN(actionCmdStr); i++) {
            if (strcmp(serverResParsed.valueAction, actionCmdStr[i]) == 0) {
                DBG_INFO("Get cmd=%s\r\n", serverResParsed.valueAction);
                switch (i)
                {
                    case ACTION_IDLE:
                        // restart the websocket streaming and recording
                        ENABLE_STREAM_AUDIO();
                        grRobot_SetLedIdle();
                        break;

                    case ACTION_DONE_WW:
                        // done wake-word, print led
                        grRobot_SetLedWwDetected();

                        // streaming command, print led
                        grRobot_SetLedRecording();

                        // websocket streaming voice command
                        ENABLE_STREAM_AUDIO();
                        break;

                    case ACTION_STREAM_CMD:
                        // streaming command, print led
                        grRobot_SetLedRecording();

                        // make sure the streaming thread working
                        ENABLE_STREAM_AUDIO();
                        break;

                    case ACTION_PREPARE_PLAYING:
                        // stop the streaming and recording
                        DISABLE_STREAM_AUDIO();

                        // set led playing audio
                        grRobot_SetLedPreparePlaying();
                        mainContinueGetAction();
                        break;

                    case ACTION_PLAY_AUDIO:
                        // stop the streaming and recording
                        DISABLE_STREAM_AUDIO();

                        // set led playing audio
                        grRobot_SetLedPlaying();

                        // start the mp3 downloader, decoder and player
                        grRobot_mp3_player();
                        mainContinueGetAction();
                        printHeadStat();
                        break;

                    case ACTION_GET_ACTION_JSON:
                        DBG_INFO(">>>>Before HttpGetAction>>>>\r\n");
                        if (grHttpGet(network, "/get-action", serverRespStr, sizeof(serverRespStr)) == false) {
                            mainContinueGetAction();
                            DBG_INFO("HTTP Get action fail\r\n");
                            Thread::wait(500);
                            continue;
                        }
                        DBG_INFO("<<<<HttpGetActionSuccess<<<<\r\n");
                        DBG_INFO("Get: %s\r\n", serverRespStr);
                        processActionEndpoint(serverRespStr);
                        break;

                    default:
                        break;
                }
            }
        }

        sysRecvMail(&mailId, &mailParams[0]);
    }
}
