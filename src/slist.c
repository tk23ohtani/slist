/*---------------------------------------------------------------------
SOURCE LIST PRINT PROGRAM      SLIST.c
COMMAND  A>slist filename [options]

Ver 1.10  ???                   ＴＡＢ機能開発
Ver 1.12  September 27,1990     メッセージ設置
Ver 1.14  April     27,1991     デバッグ
Ver 1.16  May        1,1991     デバッグ　＊日付取得
Ver 1.20  October   13,1991     オプション判定
Ver 1.21  October   15,1991     開始終了行指定
Ver 1.22  October   15,1991     ヘッダキャンセル機能
Ver 1.23  October   16,1991     Ｃソース専用モード付加
Ver 1.30  October   22,1991     関数名リストモード付加
Ver 1.31  October   23,1991     和田オプション付加
Ver 1.32  November   7,1991     UNIX漢字対応版(半角カナはまだ)
Ver 1.33  November   8,1991     UNIX漢字完全対応版
Ver 1.34  November  22,1991     デバッグ　＊ラインフィード
Ver 1.35  November  26,1992     タブ交換キャンセル機能
Ver 1.36  December  19,1992     プロトタイプ宣言が簡単
Ver 1.37  February   8,2019     for WIN32 (^^;
---------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <dos.h>
#endif // _WIN32

#define     WADA            0x0001
#define     NO_NUMBER       0x0002
#define     DIGIT_DATE      0x0004
#define     BAR_WRITE       0x0008
#define     LAST_FEED       0x0010
#define     NO_FEED         0x0020
#define     C_SOURCE        0x0040
#define     FUNC_LIST       0x0080
#define     EUCTOSJ         0x0100
#define     PROTOTYPE       0x0100

typedef     unsigned short      Word;

#ifdef _WIN32
#else // _WIN32
union REGS inregs, outregs;
#endif // _WIN32
FILE *fp, *fp2;
int   col_max = 130;
int   line_no = 1;
int   line_max = 55;
int   line_cut = 0;
int   line_st = 0;
int   line_end = 0;
int   page_cut = 1;
int   soft_tab = 4;
int   size = 256;
int   func_len;
int   iyear;
int   imonth;
int   iday;
char *p;
char headt[] = "FILE NAME = ";
char *headf;
char headd[10];
char fname[80];
char mon[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nor", "Dec" };
unsigned int    fg = 0x0000;

/*---------------------------------------------------------------------
プロトタイプ宣言

---------------------------------------------------------------------*/
Word euctosj(Word, Word);
void init(void);
void page_change(void);
void string(char, int);
void get_date(void);
char *(get_fname(char *));
int func_cnt(char);
void func_name(void);

/*---------------------------------------------------------------------
主関数

---------------------------------------------------------------------*/
void main(int argc, char *argv[])
{
    int     i, j, k;
    unsigned char str[256];
    Word    euctosj(), c;

    fprintf(stderr, "\nSouce File Printer  Version 1.37\n"
        " Copyright (C) 1988-2019 "
        "\x1b[33mtk23ohtani\x1b[m\n\n");

    if (argc < 2) {
        fprintf(stderr,
            "Usage : SLIST file_name [options]\n"
            " -(or/)options :\n"
            "  Tn : TABのSPC交換数 Default=4 (0の時は無交換)\n"
            "  N  : 行番号を付ない\n"
            "  Ln : １ページの行数 Default=55\n"
            "  8  : 80行モード\n"
            "  D  : mm/dd/yyyy mode\n"
            "  B  : bar write mode\n"
            "  F  : 最後に改頁\n"
            "  Sn : 開始行指定\n"
            "  En : 終了行指定\n"
            "  H  : ページフィードしない\n"
            "  C  : Ｃ言語ソース専用\n"
            "  K  : 関数名のリストアップ\n"
            "  W  : 和田君互換版\n"
            "  U  : UNIX漢字対応(EUC to SJIS)\n"
            "  P  : プロトタイプが簡単\n"
            );
        exit(2);
    }

    for (i = 1; i<argc; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/')
            switch (argv[i][1]) {
            case    't':
            case    'T':
                soft_tab = atoi(argv[i] + 2);
                break;
            case    'l':
            case    'L':
                line_max = atoi(argv[i] + 2);
                break;
            case    '8':
                col_max = 75;
                break;
            case    'n':
            case    'N':
                fg |= NO_NUMBER;            /* non行番号フラグセット */
                break;
            case    'd':
            case    'D':
                fg |= DIGIT_DATE;           /* 月/日/年モード */
                break;
            case    'b':
            case    'B':
                fg |= BAR_WRITE;            /* -----を書く */
                break;
            case    'f':
            case    'F':
                fg |= LAST_FEED;            /* 最後に改ページする */
                break;
            case    'h':
            case    'H':
                line_cut = 1;
                fg |= NO_FEED;          /* ヘッダを発生しない */
                break;
            case    'c':
            case    'C':
                fg |= C_SOURCE;         /* Ｃソースモード */
                break;
            case    's':
            case    'S':
                line_st = atoi(argv[i] + 2);
                break;
            case    'e':
            case    'E':
                line_end = atoi(argv[i] + 2);
                break;
            case    'k':
            case    'K':
                fg |= FUNC_LIST;
                break;
            case    'w':
            case    'W':
                fg |= WADA;
                break;
            case    'u':
            case    'U':
                fg |= EUCTOSJ;
                break;
            case    'p':        /* Option -K -N -H set */
            case    'P':
                line_cut = 1;
                fg |= NO_NUMBER | NO_FEED | FUNC_LIST | PROTOTYPE;
                break;
            default:
                fprintf(stderr, "オプションの指定が違います。\n");
                exit(1);
        }
        else j = i;
    }
    strcpy(fname, argv[j]);
    if ((fp = fopen(fname, "r")) == 0) {
        fprintf(stderr, "アクセスを拒否されました。\n"); exit(1);
    }
    if (fg & WADA) col_max -= 2;
    if (fg & (C_SOURCE | FUNC_LIST)) fp2 = fopen(fname, "r");
    if (fg & FUNC_LIST) {
        line_st = 0;
        fg &= ~C_SOURCE;
    }
    if (line_st>0)
        do{
            if (fg & C_SOURCE) p = fgets(str, size, fp2);
            line_no++;
        } while ((p = fgets(str, size, fp)) != NULL && line_no<line_st);
        if (fg & C_SOURCE) func_len = func_cnt('}');
        if (fg & FUNC_LIST) func_name();
        init();
        while ((p = fgets(str, size, fp)) != NULL) {
            if (line_end>0 && line_no>line_end) {
                if (fg & FUNC_LIST) {
                    func_name();
                    if (!(fg & PROTOTYPE)) printf("\n");
                    line_cut++;
                }
                else
                    fclose(fp);
                continue;
            }
            if (line_st>line_no) {
                line_no++;
                continue;
            }
            if (line_cut == 0) {
                page_change();
                page_cut++;
            }
            if (!(fg & NO_NUMBER)) {
                printf("%4d ", line_no);
                if (fg & WADA)
                    printf(": ");
            }
            i = k = 0;
            while (str[i]) {
                if (soft_tab > 0 && str[i] == 0x09) {
                    for (j = k%soft_tab; j<soft_tab; j++) {
                        printf("%c", 0x20); k++;
                    }
                }
                else if (fg & EUCTOSJ && str[i] >= 0xa1 && str[i] <= 0xfe) {
                    c = euctosj(str[i], str[i + 1]);
                    printf("%c%c", c >> 8, c & 0xff);
                    i++; k += 2;
                }
                else {
                    if (fg & EUCTOSJ && str[i] == 0x8e)
                        i++;
                    printf("%c", str[i]); k++;
                }
                if (str[i] == 0x0a)
                    break;
                if (k >= col_max) {
                    printf("\n");
                    line_cut++; k = 0;
                }
                i++;
            }
            line_no++;
            if (fg & NO_FEED) continue;
            line_cut++;
            if (fg & C_SOURCE)
                if (--func_len == 0) {
                    func_len = func_cnt('}');
                    if (line_max - line_cut<func_len)
                        line_cut = line_max + 1;
                }
            if (line_cut >= line_max) {
                line_cut = 0;
                printf("\f");
            }
        }
        fcloseall();
        if (fg & LAST_FEED) printf("\n\f");
}
/*---------------------------------------------------------------------
ＥＵＣをＳＪＩＳに変換

---------------------------------------------------------------------*/
Word euctosj(Word uh, Word ul)
{
    uh -= 0xa1;
    ul -= 0xa1;
    if (uh > 0xfe - 0xa1 || ul > 0xfe - 0xa1) return (0);
    else if (uh & 1) ul += 0x9f;
    else {
        ul += 0x40;
        if (ul > 0x7e) ++ul;
    }
    uh = (uh >> 1) + 0x81;
    if (uh > 0x9f) uh += -0xa0 + 0xe0;
    return ((uh << 8) + ul);
}
/*---------------------------------------------------------------------


---------------------------------------------------------------------*/
void init(void)
{
    char *(get_fname());

    headf = get_fname(fname);
    get_date();
}
/*---------------------------------------------------------------------


---------------------------------------------------------------------*/
void page_change(void)
{
    char uline[80];

    printf("%s%-20s", headt, headf);
    string(' ', col_max - 62);
    if (fg & DIGIT_DATE) printf("%2d/%2d/", imonth, iday);
    else printf("%s %2d,", mon[imonth - 1], iday);
    printf("%4d       PAGE : %5d    \n", iyear, page_cut);
    if (fg & BAR_WRITE) string('-', col_max + 3);
    printf("\n");
}
/*---------------------------------------------------------------------


---------------------------------------------------------------------*/
void string(char c, int len)
{
    int i;
    for (i = 0; i<len; i++)
        putchar(c);
}
/*---------------------------------------------------------------------
日付を得る

---------------------------------------------------------------------*/
void get_date(void)
{
#ifdef _WIN32
    SYSTEMTIME stTime;
    GetLocalTime(&stTime);
    iyear = stTime.wYear;
    imonth = stTime.wMonth;
    iday = stTime.wDay;
#else // _WIN32
    inregs.h.ah = 0x2a;
    intdos(&inregs, &outregs);
    iyear = outregs.x.cx;
    imonth = outregs.h.dh;
    iday = outregs.h.dl;
#endif // _WIN32
}
/*---------------------------------------------------------------------
ファイル名だけ取り出す

---------------------------------------------------------------------*/
char *(get_fname(char *name))
{
    int i = 0;
    char    en = 0x5c;

    i = strlen(name);
    for (--i; i>0; i--){
        if (*(name + i) == en || *(name + i) == ':'){
            i++;
            break;
        }
    }
    return(name + i);
}
/*---------------------------------------------------------------------
関数などの長さの勘定

---------------------------------------------------------------------*/
int func_cnt(char c)
{
    int     cn = 0;
    char    str[256];

    while (fgets(str, size, fp2) != NULL) {
        cn++;
        if (str[0] == c)        /* 先頭の文字にcが出てくるまでの行数 */
            return cn;
    }
    return EOF;
}
/*---------------------------------------------------------------------
関数名の行番号領域指定

---------------------------------------------------------------------*/
void func_name(void)
{
    fpos_t  pos;
    int     i, cn;
    char    str[256];

    fgetpos(fp, &pos);
    fsetpos(fp2, &pos);
    if ((cn = func_cnt('{')) == EOF) {
        fclose(fp);
        return;
    }
    line_end = line_no + cn - 2;
    fgetpos(fp, &pos);
    fsetpos(fp2, &pos);
    for (cn = line_no; cn <= line_end; cn++) {
        fgets(str, size, fp2);
        for (i = 0; str[i]; i++) {
            if (str[i] == ')') {
                line_st = cn;
                break;
            }
        }
    }
}

