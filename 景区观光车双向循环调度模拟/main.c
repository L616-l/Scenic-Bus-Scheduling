/* 景区观光车双向循环调度模拟：按录入顺序构成环形路线。 */
#define _CRT_SECURE_NO_WARNINGS
/* 输入输出、字符串处理及整数范围检查所需的头文件。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
/* Windows 控制台编码设置需要的头文件。 */
#ifdef _WIN32
#include <windows.h>
#endif

/* 固定数组容量上限，实际站点数和停靠次数不能超过它们。 */
#define MAX_STATIONS 100
#define MAX_STOPS 1000

/* Station 表示一个站点，下面每个变量都是它的一项属性。 */
typedef struct {
    int stationId;              /* 站点编号 */
    char stationName[100];      /* 站点名称 */
    int waitingCount;         /* 当前等候人数 */
    int isClosed;          /* 0表示开放，1表示关闭 */
    int visitCount;          /* 已完成的停靠次数 */
} Station;

/* Record 保存一次已完成停靠的结果。 */
typedef struct {
    int stationIndex;         /* 停靠站点在数组中的下标 */
    int direction;       /* 1表示正向，-1表示反向 */
    int up, down;        /* 上车人数、下车人数 */
    int remainingWaiting;       /* 本次停靠后剩余等候人数 */
} Record;

/* 两个数组分别保存站点和停靠记录，下标从 0 开始。 */
Station stations[MAX_STATIONS];
Record records[MAX_STOPS];
int P, M, N, Q;          /* 站点数、正向步数、反向步数、最多停靠次数 */
int openStationCount = 0;       /* 开放站点数量 */
int completedStops = 0;       /* 已完成的停靠次数 */
int passengersOnboard = 0;         /* 当前车上人数 */

/* 按行读入到inputText，bufferSize 为缓冲区大小；成功返回1，结束返回0，过长返回-1。过长时丢弃本行剩余字符。 */
int readLine(char inputText[], int bufferSize) {
    int character;
    size_t textLength;
    /* 按行读取，最多存入 bufferSize - 1 个字符；NULL 表示不能继续读取。 */
    if (fgets(inputText, bufferSize, stdin) == NULL) return 0;
    textLength = strlen(inputText);
    /* 去掉行末换行符；若一行过长，则清除剩余字符并提示重输。 */
    if (textLength > 0 && inputText[textLength - 1] == '\n') {
        inputText[--textLength] = '\0';
    } else if (!feof(stdin)) {
        while ((character = getchar()) != '\n' && character != EOF) { }
        puts("输入过长，请重新输入。");
        return -1;
    }
    if (textLength > 0 && inputText[textLength - 1] == '\r') inputText[textLength - 1] = '\0';
    return 1;
}

/* 显示 message 并读整数，范围为 minValue 到 maxValue，结果放入outputValue；成功返回1，输入结束返回0。非法行重新输入。 */
int readNumber(const char *message, int minValue, int maxValue, int *outputValue) {
    char inputText[128], *endPointer;
    long parsedNumber;
    int inputStatus;
    while (1) {
        printf("%s", message);
        inputStatus = readLine(inputText, sizeof(inputText));
        if (inputStatus == 0) return 0;
        if (inputStatus == -1) continue;
        /* strtol 将字符串转换为十进制整数，endPointer 指向未转换的部分。 */
        errno = 0;
        parsedNumber = strtol(inputText, &endPointer, 10);
        if (endPointer != inputText) {
            /* 数字后面允许空格或制表符，但不能有其他字符。 */
            while (*endPointer == ' ' || *endPointer == '\t') endPointer++;
            if (*endPointer == '\0' && errno != ERANGE && parsedNumber >= minValue && parsedNumber <= maxValue) {
                /* 通过 outputValue 将检查合格的数值交给调用这个函数的位置。 */
                *outputValue = (int)parsedNumber;
                return 1;
            }
        }
        printf("请输入 %d 到 %d 之间的整数。\n", minValue, maxValue);
    }
}

/* 在前 searchCount 个站点中按 stationId 顺序查找；找到返回数组下标，否则返回-1。 */
int findStation(int stationId, int searchCount) {
    int loopIndex;
    /* 从前往后逐一比较，只查找已经录入的前 searchCount 个站点。 */
    for (loopIndex = 0; loopIndex < searchCount; loopIndex++) {
        if (stations[loopIndex].stationId == stationId) return loopIndex;
    }
    return -1;
}

/* 无参数；录入参数和站点，检查范围及重复编号。成功返回1，输入结束返回0。 */
int inputData(void) {
    int loopIndex, inputStatus, charIndex, hasNonSpace;
    /* 每次读取一项，输入结束时返回 0，不继续使用不完整数据。 */
    if (!readNumber("站点数 P（1~100）：", 1, MAX_STATIONS, &P)) return 0;
    if (!readNumber("正向步数 M（1~1000000）：", 1, 1000000, &M)) return 0;
    if (!readNumber("反向步数 N（1~1000000）：", 1, 1000000, &N)) return 0;
    if (!readNumber("最多停靠次数 Q（0~1000）：", 0, MAX_STOPS, &Q)) return 0;

    /* 按路线的正向顺序逐个录入站点。 */
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        printf("\n--- 录入第 %d 个站点 ---\n", loopIndex + 1);
        while (1) {
            if (!readNumber("站点编号（非负整数）：", 0, INT_MAX, &stations[loopIndex].stationId)) return 0;
            /* 在前 loopIndex 个站点中找不到相同编号时，才能继续录入。 */
            if (findStation(stations[loopIndex].stationId, loopIndex) == -1) break;
            puts("站点编号重复，请重新输入。");
        }
        while (1) {
            printf("站点名称：");
            inputStatus = readLine(stations[loopIndex].stationName, sizeof(stations[loopIndex].stationName));
            if (inputStatus == 0) return 0;
            /* hasNonSpace 用来判断名称是否包含至少一个非空白字符。 */
            hasNonSpace = 0;
            if (inputStatus == 1) {
                for (charIndex = 0; stations[loopIndex].stationName[charIndex] != '\0'; charIndex++) {
                    if (!isspace((unsigned char)stations[loopIndex].stationName[charIndex])) hasNonSpace = 1;
                }
            }
            if (hasNonSpace) break;
            puts("名称不能为空，请重新输入。");
        }
        if (!readNumber("初始等候人数：", 0, INT_MAX, &stations[loopIndex].waitingCount)) return 0;
        if (!readNumber("是否关闭（0开放，1关闭）：", 0, 1, &stations[loopIndex].isClosed)) return 0;
        /* 统计开放站点的数量，后面要用它判断是否可以调度。 */
        if (stations[loopIndex].isClosed == 0) openStationCount++;
    }
    return 1;
}

/* 从 currentStation 沿 direction 寻找下一个开放站点；返回其下标，整圈找不到返回-1。先移动再检查。 */
int nextOpenStation(int currentStation, int direction) {
    int loopIndex;
    /* 最多查找一整圈；先移动，再判断该位置是否开放。
       加 P 避免反向移动时出现负数，取余使下标在首尾之间循环。 */
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        currentStation = (currentStation + direction + P) % P;  /* 处理环首、环尾的衔接 */
        /* 如果关闭则继续找；只有开放站点才作为结果返回。 */
        if (stations[currentStation].isClosed == 0) return currentStation;
    }
    return -1;
}

/* 在 currentStation 站点按 direction 完成上下客；成功返回1，输入结束返回0。全部输入成功后才更新和记录。 */
int serveStation(int currentStation, int direction) {
    int up, down, boardingLimit;
    printf("\n第 %d 次：%s到达 [%d] %s，等候 %d 人，车上 %d 人。\n",
           completedStops + 1, direction == 1 ? "正向" : "反向",
           stations[currentStation].stationId, stations[currentStation].stationName, stations[currentStation].waitingCount, passengersOnboard);
    /* 下车人数必须在 0 到车上人数之间。 */
    if (!readNumber("下车人数：", 0, passengersOnboard, &down)) return 0;
    /* 上车人数不能超过等候人数，也不能使车上人数超过整数上限。 */
    /* 先求车上还能容纳多少个 int 范围内的人，再与等候人数比较。 */
    boardingLimit = INT_MAX - (passengersOnboard - down);
    if (boardingLimit > stations[currentStation].waitingCount) boardingLimit = stations[currentStation].waitingCount;
    if (!readNumber("上车人数：", 0, boardingLimit, &up)) return 0;

    /* 两项输入都成功后才更新人数；中途结束时不计入此次停靠。 */
    passengersOnboard = passengersOnboard - down + up;
    stations[currentStation].waitingCount -= up;  /* 下车游客离开站点，不加入等候人数 */
    /* 只统计完成上下客的停靠，经过站点不计入。 */
    stations[currentStation].visitCount++;
    /* completedStops 既表示已完成次数，也是下一条记录的下标。 */
    records[completedStops].stationIndex = currentStation;
    records[completedStops].direction = direction;
    records[completedStops].up = up;
    records[completedStops].down = down;
    records[completedStops].remainingWaiting = stations[currentStation].waitingCount;
    completedStops++;
    return 1;
}

/* 无参数；从起点交替调度至Q次。正常结束返回1，输入结束或无开放站点返回0。 */
int dispatch(void) {
    /* currentStation 保存当前位置下标；方向 1 为正向，-1 为反向。 */
    int stationId, currentStation, direction = 1, stepsToMove, loopIndex;
    /* Q 为 0 时没有停靠，不必输入起点。 */
    if (Q == 0) {
        puts("Q为0，不进行调度。请查看空记录和统计结果。");
        return 1;
    }
    /* Q 大于 0 时至少要有一个开放站点。 */
    if (openStationCount == 0) {
        puts("异常：遍历一圈找不到开放站点，调度终止。");
        return 0;
    }
    while (1) {
        if (!readNumber("\n起点站点编号：", 0, INT_MAX, &stationId)) return 0;
        /* 起点既要存在，也必须开放；否则重新输入。 */
        currentStation = findStation(stationId, P);
        if (currentStation != -1 && stations[currentStation].isClosed == 0) break;
        puts("起点不存在或已关闭，请重新输入。");
    }
    puts("初始车上人数为0；离开起点后才开始计数。");
    /* 按照方向决定本次需要经过多少个开放站点。 */
    while (completedStops < Q) {
        if (direction == 1) stepsToMove = M;
        else stepsToMove = N;

        /* 每经过 openStationCount 个开放站点恰好走一圈，可减少重复扫描。 */
        /* 例如有 3 个开放站点，走 6 步和走 3 步的终点相同。
           此公式保留至少一步，整圈时也能回到当前站点。 */
        stepsToMove = (stepsToMove - 1) % openStationCount + 1;
        /* 每次寻找下一个开放站点，起点本身不直接计入步数。 */
        for (loopIndex = 0; loopIndex < stepsToMove; loopIndex++) {
            currentStation = nextOpenStation(currentStation, direction);
            if (currentStation == -1) {
                puts("异常：遍历一圈找不到开放站点，调度终止。");
                return 0;
            }
        }
        /* 找到目标后办理上下客；输入结束则中止本次停靠。 */
        if (!serveStation(currentStation, direction)) {
            puts("\n输入结束，本次停靠未完成。");
            return 0;
        }
        direction = -direction;  /* 完成上下客后调转方向 */
    }
    return 1;
}

/* 无参数、无返回值；顺序输出已完成记录、序列、未停靠站点和访问次数。 */
void printResults(void) {
    int loopIndex, stationIndex, foundStation = 0;
    /* completedStops 只包含已经完成的记录，按实际顺序输出。 */
    printf("\n=== 调度结果：共完成 %d 次停靠 ===\n", completedStops);
    if (completedStops == 0) puts("没有完成停靠。");
    for (loopIndex = 0; loopIndex < completedStops; loopIndex++) {
        stationIndex = records[loopIndex].stationIndex;
        printf("%d. %s [%d] %s，上车 %d，下车 %d，剩余等候 %d\n",
               loopIndex + 1, records[loopIndex].direction == 1 ? "正向" : "反向",
               stations[stationIndex].stationId, stations[stationIndex].stationName,
               records[loopIndex].up, records[loopIndex].down, records[loopIndex].remainingWaiting);
    }
    printf("\n完整停靠站点序列：");
    if (completedStops == 0) printf("（空）");
    for (loopIndex = 0; loopIndex < completedStops; loopIndex++) {
        stationIndex = records[loopIndex].stationIndex;
        printf("%s[%d] %s", loopIndex ? " -> " : "", stations[stationIndex].stationId, stations[stationIndex].stationName);
    }
    printf("\n");
    /* foundStation 标记是否找到未停靠的开放站点。 */
    puts("\n未停靠过的开放站点：");
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        if (stations[loopIndex].isClosed == 0 && stations[loopIndex].visitCount == 0) {
            printf("[%d] %s\n", stations[loopIndex].stationId, stations[loopIndex].stationName);
            foundStation = 1;
        }
    }
    if (foundStation == 0) puts("无。");
    puts("\n各站点访问频次：");
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        printf("[%d] %s：%d 次", stations[loopIndex].stationId, stations[loopIndex].stationName, stations[loopIndex].visitCount);
        if (stations[loopIndex].isClosed == 1) printf("（关闭）");
        printf("\n");
    }
    printf("车上剩余人数：%d\n", passengersOnboard);
}

/* 无参数、无返回值；两次遍历开放站点，输出最高、最低频次及全部并列站点。 */
void printExtremes(void) {
    int loopIndex, minValue = INT_MAX, maxValue = 0;
    if (openStationCount == 0) {
        puts("没有开放站点，无法统计最高和最低频次。");
        return;
    }
    /* 先找出所有开放站点中的最小和最大访问次数。 */
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        if (stations[loopIndex].isClosed == 0) {
            if (stations[loopIndex].visitCount < minValue) minValue = stations[loopIndex].visitCount;
            if (stations[loopIndex].visitCount > maxValue) maxValue = stations[loopIndex].visitCount;
        }
    }
    /* 再遍历站点输出与最大或最小值相等的全部并列站点。 */
    printf("\n开放站点最高频次（%d 次）：\n", maxValue);
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        if (stations[loopIndex].isClosed == 0 && stations[loopIndex].visitCount == maxValue)
            printf("[%d] %s\n", stations[loopIndex].stationId, stations[loopIndex].stationName);
    }
    printf("开放站点最低频次（%d 次）：\n", minValue);
    for (loopIndex = 0; loopIndex < P; loopIndex++) {
        if (stations[loopIndex].isClosed == 0 && stations[loopIndex].visitCount == minValue)
            printf("[%d] %s\n", stations[loopIndex].stationId, stations[loopIndex].stationName);
    }
}

/* 程序入口：依次录入信息、执行调度并输出结果。 */
int main(void) {
    int dispatchSucceeded;
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    puts("=== 景区观光车双向循环调度模拟 ===");
    puts("请按环形正向顺序录入站点。");
    if (!inputData()) {
        puts("\n输入结束，程序退出。");
        return 1;
    }
    /* 即使调度中途停止，也输出之前已经完成的记录。 */
    dispatchSucceeded = dispatch();
    printResults();
    printExtremes();
    if (dispatchSucceeded) return 0;
    return 1;
}
