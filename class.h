#include <math.h>
#include <windows.h>

#define WIN_W 1000  // 1000
#define WIN_H 666   // 800
#define MAXDIP 3000 // ���������������
#define MAXIMG 20   // һ����ɫ����ͼƬ����
#define PI 3.141592

// ��Ϸ����
#define FORM_START 0
#define FORM_GAME 1
#define FORM_END 2

// ʱ�����
#define TIME_NOTHING -2
#define TIME_SECOND -1
#define TIME_OUT 0

// ��Ϸ�غ�
#define ROUND_MYTURN 0  // ��ɫ
#define ROUND_P2TURN 1  // ��һ����
#define ROUND_MONSTER 2 // ����
#define ROUND_OTHER 3

// ��Ϸ����
#define GAME_MYTURN 0
#define GAME_MOVING 1
#define GAME_ACCUMULATING 2  // ����
#define GAME_ATTACKING 3     // ׼������
#define GAME_THROWING 4      // �ӳ�����
#define GAME_FLYING 5        // ��������
#define GAME_BOMBING 6       // �ҵ���
#define GAME_EXTINGUISHING 7 // �ҵ�����

#define DLEFT 0               // ������
#define DRIGHT 1              // ������
#define SLOT_SMALL_HEIGHT 8   // Ѫ�۸߶�
#define SLOT_SMALL_INTERVAL 5 // Ѫ�ۼ��
#define SLOT_LARGE_HEIGHT 12  // Ѫ�۸߶�
#define SLOT_LARGE_INTERVAL 2 // Ѫ�ۼ��

// �غϿ���
#define TURN_DEADLINE 20
#define TURN_NO_STATIC 0
#define TURN_INIT 1

// �˺�
#define DAMAGE_BC -3     // ���б�������
#define DAMAGE_BOW -2    // ����
#define DAMAGE_CRIT -1   // ����
#define DAMAGE_DEFAULT 0 // Ĭ��
#define DAMAGE_BLOCK 1   // ��
#define DAMAGE_IMMU 2    // ����

int PVP = true;


/**
 * ========== ���д����� ==========
 */

class Wind // ���ڴ�С
{
public:
    Wind()
    {
        w = WIN_W;
        h = WIN_H;
    }
    Wind(int ww, int hh)
    {
        w = ww;
        h = hh;
    }
    ~Wind()
    {
        ;
    }

    int l, r, t, b, w, h;
};

class KeyMouseControler // ������Ϣ
{
public:
    KeyMouseControler()
    {
        x = y = px = py = 0;
        pressing = false;
    }
    ~KeyMouseControler() {}
    int isPressing()
    {
        return pressing;
    }

    int x, y;
    int px, py;
    bool pressing;
    int nkey; // ��ǰ���µİ���

};

class TimeControler
{
public :
    TimeControler()
    {
        maxSecond = nowSecond = nowSecondProc = 0;
        pausing = false;

        hf = CreateFont(35, 20, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "����"); // (HFONT) GetStockObject(0)
    }

    ~TimeControler()
    {
        DeleteObject(hf);
    }

    void setDeadline(int second) // ���ʱ�޼����ض��л�
    {
        if (second > 0)
        {
            maxSecond = second;
            nowSecond = nowSecondProc = 0;
        }
    }

    bool isNextFrame() // �ж�ʱ���ǲ��ǹ���һ֡
    {
        return frameNow - framePre >= 40;
    }

    void updatePre() // ÿ�� MyPaint �󵽴���һ֡
    {
        framePre = GetTickCount();
    }

    int getTimeState()
    {
        if (pausing == true)
            return TIME_NOTHING;
        if (++nowSecondProc < 25)
            return TIME_NOTHING;
        nowSecondProc = 0;
        nowSecond++;
        if (nowSecond > maxSecond)
            return TIME_OUT;
        return TIME_NOTHING/*TIME_SECOND*/;
    }

    void timePause() // ֹͣ��ʱ�����������ʱ��
    {
        pausing = true;
    }

    void timeStart() // ������ʱ
    {
        pausing = false;
    }

    void BitB(HDC mdc, HDC bufdc)
    {
        // ��������ʱ��
        SelectObject(mdc, hf); // �󶨻���
        SetTextColor(mdc, RGB(255, 0, 0));
        SetBkColor(mdc, RGB(0, 0, 0));
        SetBkMode(mdc, TRANSPARENT);
        if (maxSecond - nowSecond > 10)
            SetTextColor(mdc, 0x00FF00/*BGR*/); // ����ɫ
        else if (maxSecond - nowSecond > 5)
            SetTextColor(mdc, 0x00FFFF/*BGR*/); // ����ɫ
        else
            SetTextColor(mdc, 0x0000FF/*BGR*/); // �󶨺�ɫ

        char str[100] = "";
        sprintf(str, "%d", maxSecond - nowSecond);
        TextOut(mdc, 50, 50, str, strlen(str));
    }

    DWORD framePre, frameNow; // ÿ�� 40s һ��ˢ��
    int maxSecond, nowSecond, nowSecondProc;
    HFONT hf;
    bool pausing;
};

class TurnControler
{
public :
    TurnControler()
    {
        form = FORM_START; //��Ϸ����
        round = ROUND_MYTURN; //��Ϸ�غ�
        proc = GAME_MYTURN; //�غϽ���

        hf = CreateFont(20, 10, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH, "����");
    }

    ~TurnControler()
    {
        ;
    }

    int next() // ��ʱ���˻��߲�������ɣ�����һ�غϲ����ض�ʱ
    {
        turnStatic = TURN_NO_STATIC;
        if (form == FORM_GAME)
        {
            if (round == ROUND_MYTURN) // �Լ��Ļغϣ��ж϶���
            {
                if (PVP)
                    round = ROUND_P2TURN;
                else
                    round = ROUND_MONSTER;
                return TURN_DEADLINE;
            }
            else if (round == ROUND_P2TURN)
            {
                round = ROUND_MYTURN;
                return TURN_DEADLINE;
            }
            else if (round == ROUND_MONSTER) // ����Ļغ�
            {
                round = ROUND_MYTURN;
                return TURN_DEADLINE;
            }
            else/* if (round == ROUND_OTHER) */ // ���� δ֪��
            {
                round = ROUND_MYTURN;
                return TURN_DEADLINE;
            }
        }
        else if (form == FORM_START)
        {
            form = FORM_GAME;
            round = GAME_MYTURN;
            return TURN_DEADLINE;
        }
        else if (form == FORM_END)
        {
            form = FORM_START;
            return 5;
        }
        return 0;
    }

    void BitB(HDC mdc)
    {
        // ��������ʱ��
        SelectObject(mdc, hf); // �󶨻���
        SetTextColor(mdc, RGB(255, 0, 255));
        SetBkMode(mdc, TRANSPARENT);
        SetTextColor(mdc, 0x0000FF/*BGR*/); // �󶨺�ɫ

        char str[100] = "";
        if (round == ROUND_MYTURN)
            strcpy(str, "p1");
        else if (round == ROUND_P2TURN)
            strcpy(str, "p2");
        else if (round == ROUND_MONSTER)
            strcpy(str, "monster");
        else strcpy(str, "");
        TextOut(mdc, 0, 0, str, strlen(str));
    }


    int form;        // ��Ϸ����
    int round;       // ��Ϸ�غ�
    int proc;        // �غϽ���
    int turnStatic; // �Ƿ���Ҫ

    HFONT hf;
};

class WindControl
{
public:
    WindControl();
    ~WindControl();

};

class Form
{
public:
    Form();
    ~Form();

    int x, y, w, h; // λ��

};

class StartForm : public Form
{
public:
    StartForm();
    ~StartForm();

};

class GameForm : public Form
{
public:
    GameForm();
    ~GameForm();

};

class EndForm : public Form
{
public:
    EndForm()
    {
        fontSize = 0;
    }
    ~EndForm();

    int fontSize;
};


/**
 * ========== ������ ==========
 */

class Static
{
public:

    static int getBmpW(char *name)
    {
        if (strcmp(name, "bg_rainbow") == 0)
            return 1000;
        if (strcmp(name, "bg_start") == 0)
            return 1000;
        if (strcmp(name, "bg_ghost") == 0)
            return 1000;
        if (strcmp(name, "role") == 0 || strcmp(name, "role_r") == 0)
            return 40;
        if (strcmp(name, "role2") == 0 || strcmp(name, "role2_r") == 0)
            return 40;
        if (strcmp(name, "tri_darts") == 0 || strcmp(name, "tri_darts_r") == 0)
            return 30;
        if (strcmp(name, "tri_darts_bomb") == 0 || strcmp(name, "tri_darts_bomb_r") == 0)
            return 40;
        if (strcmp(name, "ice_cream") == 0 || strcmp(name, "ice_cream_r") == 0)
            return 30;
        if (strcmp(name, "ice_cream_bomb") == 0 || strcmp(name, "ice_cream_bomb_r") == 0)
            return 40;
        if (strcmp(name, "bow") == 0)
            return 80;
        if (strcmp(name, "fly") == 0)
            return 40;
        if (strcmp(name, "flyAttack") == 0)
            return 40;

        return 100;
    };

    static int getBmpH(char *name)
    {
        if (strcmp(name, "bg_rainbow") == 0)
            return 600;
        if (strcmp(name, "bg_start") == 0)
            return 660;
        if (strcmp(name, "bg_ghost") == 0)
            return 600;
        if (strcmp(name, "role") == 0 || strcmp(name, "role_r") == 0)
            return 40;
        if (strcmp(name, "role2") == 0 || strcmp(name, "role2_r") == 0)
            return 59;
        if (strcmp(name, "tri_darts") == 0 || strcmp(name, "tri_darts_r") == 0)
            return 30;
        if (strcmp(name, "tri_darts_bomb") == 0 || strcmp(name, "tri_darts_bomb_r") == 0)
            return 40;
        if (strcmp(name, "ice_cream") == 0 || strcmp(name, "ice_cream_r") == 0)
            return 30;
        if (strcmp(name, "ice_cream_bomb") == 0 || strcmp(name, "ice_cream_bomb_r") == 0)
            return 40;
        if (strcmp(name, "bow") == 0)
            return 80;
        if (strcmp(name, "fly") == 0)
            return 40;
        if (strcmp(name, "flyAttack") == 0)
            return 40;

        return 100;
    };

};

/**
 * Ѫ����һ��Ķ���
 */

class Slot
{
public:
    Slot()
    {
        //hbr_fore = (HBRUSH) CreateSolidBrush(RGB(fore_r, fore_g, fore_b));
        //hbr_back = (HBRUSH) CreateSolidBrush(RGB(back_r, back_g, back_b));
        setBackColor(127, 0, 0);
        setForeColor(255, 0, 0);
        setBackColor(0, 0, 127);
        setForeColor(0, 0, 255);
    }
    Slot(int ww, int hh)
    {
        Slot();
        setVal(ww, hh, 0);
    }
    Slot(int ww, int hh, int mm/*maxpos*/)
    {
        Slot();
        setVal(ww, hh, mm);
    }
    ~Slot()
    {
        DeleteObject(hbr_fore);
        DeleteObject(hbr_back);
    }

    void setVal(int ww, int hh, int mm)
    {
        width = ww;
        height = hh;
        maxpos = mm;
    }

    void setForeColor(int r, int g, int b) // ����ǰ��ɫ��һ������Ѫ������ɫ
    {
        fore_r = r;
        fore_g = g;
        fore_b = b;
        hbr_fore = (HBRUSH) CreateSolidBrush(RGB(fore_r, fore_g, fore_b));
    }

    void setBackColor(int r, int g, int b) // ����ǰ��ɫ��һ������Ѫ������ɫ
    {
        back_r = r;
        back_g = g;
        back_b = b;
        hbr_back = (HBRUSH) CreateSolidBrush(RGB(back_r, back_g, back_b));
    }


    void BitB(HDC mdc, int left, int top, int pos/*��ֵ*/)
    {
        SelectObject(mdc, hbr_fore);
        Rectangle(mdc, left, top, left + width, top + height);

        SelectObject(mdc, hbr_back);
        int w = pos * width / maxpos;
        Rectangle(mdc, left, top, left + w, top + height);

    }

    void BitB(HDC mdc, int left, int top, int pos, int max)
    {
        SelectObject(mdc, hbr_fore);
        Rectangle(mdc, left, top, left + width, top + height);

        SelectObject(mdc, hbr_back);
        int w = pos * width / max;
        Rectangle(mdc, left, top, left + w, top + height);
    }

    HBRUSH hbr_fore, hbr_back;

    int width, height; // ��״
    int fore_r, fore_g, fore_b; // ǰ��ɫ
    int back_r, back_g, back_b; // ����ɫ
    int border_r, border_g, border_b; // �߿���ɫ����ʱ�ò���
    int maxpos; // �����ֵ�����ں� pos �����ı�������

};

class Animation
{
public:
    Animation()
    {
        bmpNum = bmpp = 0;
    }
    Animation(char *name)
    {
        setBmps(name);
    }
    Animation(char *name, int xx, int yy)
    {
        setBmps(name);
        x = xx;
        y = yy;
    }
    Animation(char *name, int num)
    {
        setBmps(name, num);
    }
    ~Animation()
    {
        for (int i = 0; i < bmpNum; i++)
        {
            DeleteObject(hbmps[i]);
            DeleteObject(hbmps[i]);
        }
    }

    void setBmps(char *name)  // ��������ͼ
    {
        strcpy(bmpName, name);
        bmpNum = 1;
        bmpp = 0;
        char fullName[1000];
        sprintf(fullName, "img/%s.bmp", name);
        hbmps[0] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        // GetObject(hbmps[0], sizeof(BITMAP), &bmps[0]);

        w = Static::getBmpW(name);
        h = Static::getBmpH(name);
    }

    void set_Bmps(char *name)  // ���ð�����ͼ
    {
        strcpy(bmpName, name);
        bmpNum = 1;
        bmpp = 0;
        w = Static::getBmpW(name);
        h = Static::getBmpH(name);

        char fullName[1000];
        sprintf(fullName, "img/%s.bmp", name);
        hbmps[0] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        sprintf(fullName, "img/_%s.bmp", name);
        _hbmps[0] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        GetObject(hbmps[0], sizeof(BITMAP), &bmps[0]);

        sprintf(fullName, "img/%s_r.bmp", name);
        hbmps_r[0] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        sprintf(fullName, "img/_%s_r.bmp", name);
        _hbmps_r[0] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        GetObject(hbmps_r[0], sizeof(BITMAP), &bmps_r[0]);
    }

    void setBmps(char *name, int num) // ��ͼ������һ���ǰ�����ͼ��
    {
        strcpy(bmpName, name);
        bmpNum = num;
        bmpp = 0;
        w = Static::getBmpW(name);
        h = Static::getBmpH(name);

        char fullName[100];
        for (int i = 0; i < num; i++)
        {
            sprintf(fullName, "img/%s%d.bmp", name, i);
            hbmps[i] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

            sprintf(fullName, "img/_%s%d.bmp", name, i);
            _hbmps[i] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

            GetObject(hbmps[i], sizeof(BITMAP), &bmps[i]);

            sprintf(fullName, "img/%s%d_r.bmp", name, i);
            hbmps_r[i] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

            sprintf(fullName, "img/_%s%d_r.bmp", name, i);
            _hbmps_r[i] = (HBITMAP)LoadImage(NULL, fullName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

            GetObject(hbmps_r[i], sizeof(BITMAP), &bmps_r[i]);
        }
    }

    HBITMAP getBmp() // ��ȡλͼָ��
    {
        return hbmps[bmpp];
    }

    HBITMAP getNextBmp() // ��ȡ��һ��λͼ��ָ�룬�����γɶ���
    {
        if (++bmpp >= bmpNum)
            bmpp = 0;
        return hbmps[bmpp];
    }

    HBITMAP get_Bmp()
    {
        return _hbmps[bmpp];
    }

    HBITMAP getNext_Bmp()
    {
        if (++bmpp >= bmpNum)
            bmpp = 0;
        return _hbmps[bmpp];
    }

    HBITMAP getBmp_r() // ��ȡλͼָ��
    {
        return hbmps_r[bmpp];
    }

    HBITMAP getNextBmp_r() // ��ȡ��һ��λͼ��ָ�룬�����γɶ���
    {
        if (++bmpp >= bmpNum)
            bmpp = 0;
        return hbmps_r[bmpp];
    }

    HBITMAP get_Bmp_r()
    {
        return _hbmps_r[bmpp];
    }

    HBITMAP getNext_Bmp_r()
    {
        if (++bmpp >= bmpNum)
            bmpp = 0;
        return _hbmps_r[bmpp];
    }

    void setXYV(int xx, int yy, int vxx, int vyy)
    {
        x = xx;
        y = yy;
        vx = vxx;
        vy = vyy;
    }

    void BitB1(HDC &mdc, HDC &bufdc)
    {
        SelectObject(bufdc, getBmp());
        BitBlt(mdc, x, y, w, h, bufdc, 0, 0, SRCCOPY);
    }

    void BitB(HDC &mdc, HDC &bufdc)
    {
        SelectObject(bufdc, getNext_Bmp());
        BitBlt(mdc, x, y, w, h, bufdc, 0, 0, SRCAND);
        SelectObject(bufdc, getNextBmp());
        BitBlt(mdc, x, y, w, h, bufdc, 0, 0, SRCPAINT);
    }

    void BitB_r(HDC &mdc, HDC &bufdc)
    {
        SelectObject(bufdc, getNext_Bmp_r());
        BitBlt(mdc, x, y, w, h, bufdc, 0, 0, SRCAND);
        SelectObject(bufdc, getNextBmp_r());
        BitBlt(mdc, x, y, w, h, bufdc, 0, 0, SRCPAINT);
    }

    int bmpNum, bmpp;                          // ͼ��������ͼ������
    char bmpName[30];                          // ͼ�����ƣ������ļ��С���׺����
    HBITMAP hbmps[MAXIMG], _hbmps[MAXIMG];     // ͼ��ָ��
    HBITMAP hbmps_r[MAXIMG], _hbmps_r[MAXIMG]; // ���ҵ�
    BITMAP bmps[MAXIMG], bmps_r[MAXIMG];       // ͼ�����
    int x, y, w, h;                            // ͼ��Ĵ�С
    int vx, vy;                                // ͼ����˶�
};

class BmpAni : public Animation
{
public:
    BmpAni()
    {
        Animation();
    }
    BmpAni(char *name)
    {
        Animation(name, 1);
    }
    BmpAni(char *name, int num)
    {
        Animation(name, num);
    }
    ~BmpAni()
    {

    }
};

class ThrowAni : public Animation
{
public:
    ThrowAni() {}

    ThrowAni(char *name)
    {
        Animation(name, 1);
    }

    ThrowAni(char *name, int num)
    {
        Animation(name, num);
    }

    ~ThrowAni() {}

};

class BombAni
{
public:
    BombAni()
    {
        maxnum = 0;
        for (int i = 0; i < 100; i++)
        {
            index[i] = 0;
            bow[i].setBmps("bow", 4);
        }
    }
    ~BombAni()
    {
        ;
    }

    void addBomb(int x, int y)
    {
        int i;
        for (i = 0; i < 100; i++)
            if (index[i] == 0)
                break;
        index[i] = 5;
        if (maxnum <= i)
            maxnum = i + 1;
        bow[i].setXYV(x - bow[i].w / 2, y - bow[i].h / 2, 0, 0);
    }

    void BitB(HDC mdc, HDC bufdc)
    {
        // bufdc = NULL;
        for (int i = 0; i < maxnum; i++)
        {
            if (index[i] > 0)
            {
                if (--index[i] > 0)
                {
                    SelectObject(bufdc, bow[i].get_Bmp());
                    BitBlt(mdc, bow[i].x, bow[i].y, bow[i].w, bow[i].h, bufdc, 0, 0, SRCAND);
                    SelectObject(bufdc, bow[i].getBmp());
                    BitBlt(mdc, bow[i].x, bow[i].y, bow[i].w, bow[i].h, bufdc, 0, 0, SRCPAINT);

                    // bow[i].bmpp++;
                }
                /*else
                {
                    SelectObject(bufdc, bow[i].get_Bmp());
                    BitBlt(mdc, bow[i].x, bow[i].y, bow[i].w, bow[i].h, bufdc, 0, 0, SRCAND);
                    SelectObject(bufdc, bow[i].getBmp());
                    BitBlt(mdc, bow[i].x, bow[i].y, bow[i].w, bow[i].h, bufdc, 0, 0, SRCPAINT);
                }*/

            }
        }
    }

    int maxnum;
    Animation bow[100];
    int index[100];
};


/**
 * ========== ��ͼ������ ==========
 */

class MapObj
{
public:
    MapObj()
    {
    }

    MapObj(char *name)
    {
        pics.setBmps(name, 0);
    }

    ~MapObj()
    {

    }

    void BitB(HDC mdc, HDC bufdc)
    {
        SelectObject(bufdc, pics.getNextBmp());
        BitBlt(mdc, 0, 0, Static::getBmpW(pics.bmpName), Static::getBmpH(pics.bmpName), bufdc, 0, 0, SRCCOPY);
    }

    BmpAni pics;
};

class BgMap : public MapObj
{
public:
    BgMap()
    {
        initXY();
    }
    BgMap(char *s)
    {
        pics.setBmps(s);
        initXY();
    }
    ~BgMap()
    {

    }
    void setBg(char *s)
    {
        pics.setBmps(s);
        initXY();
    }

    void initXY()
    {
        for (int i = 0; i < MAXDIP; i++)
        {
            top[i] = height[i] = top1[i] = height1[i] = top2[i] = height2[i] = top3[i] = height3[i] = WIN_H;
        }
        if (strcmp(pics.bmpName, "bg_rainbow") == 0)
        {
            for (int i = 0; i <= WIN_W; i++)
            {
                top[i] = (int)(2860 - sqrt(2380 * 2380 - (i - 511) * (i - 511)));
                height[i] = WIN_H;
            }
        }
        else if (strcmp(pics.bmpName, "bg_ghost") == 0)
        {
            int i;
            for (i = 255; i < 739; i++)
            {
                top1[i] = 56;
                height1[i] = 56;
            }
            for (i = 133; i < 872; i++)
            {
                top2[i] = 237;
                height2[i] = 70;
            }
            for (i = 0; i <= WIN_W; i++)
            {
                top[i] = 452;
                height[i] = WIN_H;
            }
            for (i = 470; i < 530; i++)
            {
                top3[i] = 1;
                height3[i] = WIN_H;
            }
        }
    }

    bool canLeft(int x, int y) // �ܷ�������
    {
        if (x <= 0 || x >= WIN_W)
            return false;
        if (top[x - 1] < top[x] - 5 && top[x - 1] + height[x - 1] > top[x] && y > top[x - 1] && y < top[x - 1] + height[x - 1])
            return false;
        return true;
    }

    bool canRight(int x, int y) // �ܷ�������
    {
        if (x <= 0 || x >= WIN_W)
            return false;
        if (top[x + 1] < top[x] - 5 && top[x + 1] + height[x + 1] > top[x] && y > top[x + 1] && y < top[x + 1] + height[x + 1])
            return false;
        return true;
    }

    int disabs(int x)
    {
        return x < 0 ? -x : x;
    }

    bool isSuspend(int x, int y) // �ж��ǲ�������
    {
        if (x <= 0 || x >= WIN_W) return false;
        if (disabs(y-top[x]) < 5 || disabs(y-top1[x]) < 5 || disabs(y-top2[x]) < 5 || disabs(y-top3[x]) < 5)
            return false;
        if (y >= top[x] && y < top[x]+height[x])
            return false;
        if (y >= top1[x] && y < top1[x]+height1[x])
            return false;
        if (y >= top2[x] && y < top2[x]+height2[x])
            return false;
        if (y >= top3[x] && y < top3[x]+height3[x])
            return false;
        return true;
    }

    int getNewY(int x, int y) // ����֮�����µ�λ��
    {
        if (x <= 0 || x >= WIN_W) return y;

        if (y >= top[x] && y <= top[x]+10)
            return top[x];
        else if (y > top[x] && y <= top[x]+height[x])
            if (top[x]+height[x] < WIN_H)
                return top[x]+height[x];
            else return top[x];

        if (y >= top1[x] && y <= top1[x]+10)
            return top1[x];
        else if (y > top1[x] && y <= top1[x]+height1[x])
            if (top1[x]+height1[x] < WIN_H)
                return top1[x]+height1[x];
            else return top1[x];

        if (y >= top2[x] && y <= top2[x]+10)
            return top2[x];
        else if (y > top2[x] && y <= top2[x]+height2[x])
            if (top2[x]+height2[x] < WIN_H)
                return top2[x]+height2[x];
            else return top2[x];

        if (y >= top3[x] && y <= top3[x]+10)
            return top3[x];
        else if (y > top3[x] && y <= top3[x]+height3[x])
            if (top3[x]+height3[x] < WIN_H)
                return top3[x]+height3[x];
            else return top3[x];

        return y;
    }

    int getUpperY(int x, int y) // ���к�ƫ��һ���λ��
    {
        if (x <= 0 || x >= WIN_W) return y;

        if (y >= top[x] && y <= top[x]+10)
            return top[x];

        else if (y > top[x] && y <= top[x]+height[x])
            return top[x];

        if (y >= top1[x] && y <= top1[x]+10)
            return top1[x];
        else if (y > top1[x] && y <= top1[x]+height1[x])
            return top1[x];

        if (y >= top2[x] && y <= top2[x]+10)
            return top2[x];
        else if (y > top2[x] && y <= top2[x]+height2[x])
            return top2[x];

        if (y >= top3[x] && y <= top3[x]+10)
            return top3[x];
        else if (y > top3[x] && y <= top3[x]+height3[x])
            return top3[x];

        return y;
    }

    int getLowerY(int x, int y) // ����֮�����µ�λ��
    {
        if (x <= 0 || x >= WIN_W) return y;

        if (y >= top[x] && y <= top[x]+10)
            return top[x];

        else if (y > top[x] && y <= top[x]+height[x])
            return top[x]+height[x];

        if (y >= top1[x] && y <= top1[x]+10)
            return top1[x];
        else if (y > top1[x] && y <= top1[x]+height1[x])
                return top1[x]+height1[x];

        if (y >= top2[x] && y <= top2[x]+10)
            return top2[x];
        else if (y > top2[x] && y <= top2[x]+height2[x])
            return top2[x]+height2[x];

        if (y >= top3[x] && y <= top3[x]+10)
            return top3[x];
        else if (y > top3[x] && y <= top3[x]+height3[x])
            return top3[x]+height3[x];

        return y;
    }

    int top[MAXDIP], height[MAXDIP]; // ��¼ÿ��x���λ��
    int top1[MAXDIP], height1[MAXDIP]; // ��¼ÿ��x���λ��
    int top2[MAXDIP], height2[MAXDIP]; // ��¼ÿ��x���λ��
    int top3[MAXDIP], height3[MAXDIP]; // ��¼ÿ��x���λ��

};

class EarthMap : public MapObj
{
public:
    EarthMap();
    ~EarthMap();

};

class PartMap : public MapObj
{
public:
    PartMap();
    ~PartMap();

};

class MaskMap : public MapObj
{
public:
    MaskMap();
    ~MaskMap();

};

/**
 * ========== װ���� ==========
 */

class EquipmentObj
{
public:
    EquipmentObj() {}
    EquipmentObj(char *pic)
    {
        setEquiBmps(pic);
    }
    ~EquipmentObj() {}

    void setEquiBmps(char *pic)
    {
        /* pics.set_Bmps(pic);
         char fullName[100];
         sprintf(fullName, "%s_r", pic);
         pics_r.set_Bmps(fullName);*/
    }

    void throwAttack(int x, int y, int vx, int vy)
    {
        ;
    }

    int value, range;
    int x, y, vx, vy, block;
    ThrowAni pics, pics_r;
};

class Arms : public EquipmentObj
{
public:
    Arms() {}
    Arms(char *name)
    {
        setEquiBmps(name);
    }
    ~Arms() {}

    void setArmBmps(char *pic)
    {
        // setEquiBmps(pic);
    }
};

class Shield : public EquipmentObj
{
public:
    Shield();
    ~Shield();

};

class Props : public EquipmentObj
{
public:
    Props();
    ~Props();

};



/**
 * ========== ������ ==========
 */

class LiveObg
{
public:
    LiveObg()
    {
        /*hp = 100;
        mp = 50; // ÿ�غϲ���
        rage = 0;
        attack = 30;
        defense = 10;
        speed = 10; // ÿ���ٶ�
        attack_BOMB = 60;
        x = y = 0;*/
        setLiveObj(100, 50, 0, 30, 10, 3, 60, 0, 0, "", "");

        hf = CreateFont(18, 9, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "����"); // (HFONT) GetStockObject(0)
    }
    LiveObg(int h, int m, int r, int a, int d, int s, int ab, int xx, int yy, char *pic)
    {
        setLiveObj(h, m, r, a, d, s, ab, x, y, pic, "");

        hf = CreateFont(18, 9, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "����"); // (HFONT) GetStockObject(0)
    }
    LiveObg(int h, int m, int r, int a, int d, int s, int ab, int xx, int yy, char *pic, char *equi)
    {
        setLiveObj(h, m, r, a, d, s, ab, x, y, pic, equi);

        hf = CreateFont(18, 9, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "����"); // (HFONT) GetStockObject(0)
    }
    ~LiveObg()
    {
        DeleteObject(hf);
    }

    void setLiveObj(int hhp, int mmp, int r, int a, int d, int s, int ab, int xx, int yy, char *pic, char *equi)
    {
        maxhp = nowhp = hp = hhp;
        maxmp = nowmp = mp = mmp; // ÿ�غ��ƶ�ʱ��
        rage = r;
        attack = a;
        defense = d;
        speed = s; // ÿ���ٶ�
        attack_BOMB = ab;
        x = xx;
        y = yy;
        w = Static::getBmpW(pic);
        h = Static::getBmpH(pic);
        pics.set_Bmps(pic);

        angle = 0;
        hpcut = 0; // �ϴο�Ѫ��
        hpcut_ms = 0; // ��Ѫʱ��
        hpstate = 0; // -3���б�����-2���У�-1������0������1�񵲣�2���ߣ�3����

        char fullName[1000] = ""; // ����ͼƬ
        sprintf(fullName, "%s_r", pic);
        pics_r.set_Bmps(fullName);
        direct = DLEFT;

        s_hp.setVal(w, SLOT_SMALL_HEIGHT, hp);
        s_mp.setVal(w, SLOT_SMALL_HEIGHT, mp);
        s_hp.setBackColor(255, 0, 0);
        s_hp.setForeColor(127, 0, 0);
        s_mp.setBackColor(0, 0, 255);
        s_mp.setForeColor(0, 0, 127);

        // arms.setArmBmps(equi);
        // eqp.setEquiBmps("1234");
    }

    void BitB(HDC &mdc, HDC &bufdc)
    {
        if (direct == DLEFT)
        {
            SelectObject(bufdc, pics.getNext_Bmp());
            BitBlt(mdc, x, y, Static::getBmpW(pics.bmpName), Static::getBmpH(pics.bmpName), bufdc, 0, 0, SRCAND);
            SelectObject(bufdc, pics.getBmp());
            BitBlt(mdc, x, y, Static::getBmpW(pics.bmpName), Static::getBmpH(pics.bmpName), bufdc, 0, 0, SRCPAINT);
        }
        else
        {
            SelectObject(bufdc, pics_r.getNext_Bmp());
            BitBlt(mdc, x, y, Static::getBmpW(pics_r.bmpName), Static::getBmpH(pics_r.bmpName), bufdc, 0, 0, SRCAND);
            SelectObject(bufdc, pics_r.getBmp());
            BitBlt(mdc, x, y, Static::getBmpW(pics_r.bmpName), Static::getBmpH(pics_r.bmpName), bufdc, 0, 0, SRCPAINT);
        }

        s_hp.BitB(mdc, x, y + h, nowhp);
        s_mp.BitB(mdc, x, y + h + SLOT_SMALL_INTERVAL + SLOT_SMALL_HEIGHT, mp);

        if (nowhp > hp) // Ѫ����һ������Ѫ����
        {
			nowhp -= maxhp / 100 + 1;
			if (nowhp < hp) nowhp = hp;
		}
        else if (nowhp < hp)
        {
			nowhp += maxhp / 100 + 1;
			if (nowhp > hp) nowhp = hp;
		}

        if (hpcut != 0 && hpcut_ms < 25) // ����Ѫ�Ķ���
        {
            hpcut_ms++;
            // ��������ʱ��
            SelectObject(mdc, hf); // �󶨻���
            SetBkMode(mdc, TRANSPARENT);
            GetTextMetrics(mdc, &tm); // �󶨴�С
            char str[100] = "";

            if (hpcut > 0) // ��Ѫ
            {
                SetTextColor(mdc, 0x0000FF/*BGR*/); // �󶨺�ɫ
                if (hpstate == DAMAGE_CRIT)
                    sprintf(str, "����-%d", hpcut);
                else if (hpstate == DAMAGE_BOW)
                    sprintf(str, "����-%d", hpcut);
                else if (hpstate == DAMAGE_BC)
                    sprintf(str, "���б���-%d", hpcut);
                else if (hpstate == DAMAGE_BLOCK)
                    sprintf(str, "��-%d", hpcut);
                else
                    sprintf(str, "-%d", hpcut);
            }
            else if (hpcut == 0 && hpstate == DAMAGE_IMMU)
            {
                SetTextColor(mdc, 0xFF0000/*BGR*/); // ����ɫ
                sprintf(str, "����");
            }
            else // ��Ѫ
            {
                SetTextColor(mdc, 0x00FF00/*BGR*/); // ����ɫ
                sprintf(str, "����+%d", -hpcut);
            }

            int len = strlen(str) * tm.tmAveCharWidth; // Ҫ���������ܳ���
            TextOut(mdc, x + w / 2 - len / 2, y - hpcut_ms * 2, str, strlen(str));
        }
        else if (hpcut_ms == 25)
        {
            hpstate = 0;
        }
    }

    bool cuthp(int damage)
    {
        hp -= damage;
        hpcut = damage;
        hpcut_ms = 0;
        if (hp <= 0)
        {
            hp = 0;
            return true; // ��Ѫ���
        }
		else if (hp > maxhp)
		{
			hp = maxhp;
		}
        return false;
    }

    HBITMAP getBmp() // ��ȡλͼָ��
    {
        if (direct == DLEFT)
            return pics.getBmp();
        else
            return pics_r.getBmp();
    }

    HBITMAP getNextBmp() // ��ȡ��һ��λͼ��ָ�룬�����γɶ���
    {
        if (direct == DLEFT)
            return pics.getNextBmp();
        else
            return pics_r.getNextBmp();
    }

    HBITMAP get_Bmp()
    {
        if (direct == DLEFT)
            return pics.get_Bmp();
        else
            return pics_r.get_Bmp();
    }

    HBITMAP getNext_Bmp()
    {
        if (direct == DLEFT)
            return pics.getNext_Bmp();
        else
            return pics_r.getNext_Bmp();
    }

    void goLEFT() // ����ת
    {
        direct = DLEFT;
    }

    void goRight() // ����ת
    {
        direct = DRIGHT;
    }

    void roundInit() // ÿ�غϳ�ʼ��
    {
        mp = maxmp;
    }

	void setThr(char * s)
	{
		strcpy(origin_thr, s);
		thr.set_Bmps(s);
	}

	void setTempThr(char * s)
	{
		thr.set_Bmps(s);
	}

	void setBombThr()
	{
		char fullName[100];
		sprintf(fullName, "%s_bomb", origin_thr);
		setTempThr(fullName);
	}

	void setOThr()
	{
		thr.set_Bmps(origin_thr);
	}

    int maxhp, hp, nowhp, maxmp, mp, nowmp, rage/*ŭ��ֵ*/;
    int attack, defense, speed, attack_BOMB;

    int x, y, w, h, vx, vy, vblock;
    int angle; // ����Ƕȣ���λ�Ƕ�
    int hpcut, hpcut_ms, hpstate;

    int direct;               // ��ɫ����Ĭ������
    BmpAni pics, pics_r;

    Slot s_hp, s_mp;

    HFONT hf; // ��Ѫ�Ļ���
    TEXTMETRIC tm;

    Animation thr;
	char origin_thr[100];

    Arms arm();
    Shield sheild();
    Props props();
};

class Role : public LiveObg
{
public:
    Role()
    {
        hp = (HPEN) GetStockObject(BLACK_PEN); // Ĭ�ϻ���

        accum.setVal(WIN_W, 30, 100);
        accum.setBackColor(255, 255, 0);
        accum.setForeColor(255, 128, 0);
    }
    Role(int h, int m, int r, int a, int d, int s, int ab, int x, int y, char *pic, char *equi)
    {
        setLiveObj(h, m, r, a, d, s, ab, x, y, pic, equi);
        hp = (HPEN) GetStockObject(BLACK_PEN); // Ĭ�ϻ���

        accum.setVal(WIN_W, 30, 100);
        accum.setBackColor(255, 255, 0);
        accum.setForeColor(255, 128, 0);

        thr.set_Bmps(equi);
    }

    ~Role()
    {
        DeleteObject(hp);
    }

    void roundInit() // ÿ�غϳ�ʼ��
    {
        LiveObg::roundInit();
        accuming = 0;
    }

    void BitB(HDC mdc, HDC bufdc)
    {
        // �����������
        LiveObg::BitB(mdc, bufdc);

        // ������ĽǶ�
        SelectObject(mdc, hp);
        int sx, sy, ex, ey, r;
        double pAngle = PI * angle / 180.0;
        r = 50;
        sx = x + Static::getBmpW(pics.bmpName) / 2;
        sy = y + Static::getBmpH(pics.bmpName) / 2;
        if (direct == DRIGHT)
        {
            ex = (int)(sx + cos(pAngle) * r);
            ey = (int)(sy - sin(pAngle) * r);
        }
        else
        {
            ex = (int)(sx - cos(pAngle) * r);
            ey = (int)(sy - sin(pAngle) * r);
        }
        MoveToEx(mdc, sx, sy, NULL);
        LineTo(mdc, ex, ey);
    }
	
    HPEN hp;
	
    Slot accum; // �����۶���
    int accuming; // ����ֵ
	
    bool useBomb, useFly, useAid, useProtect;
};

class Monster : public LiveObg
{
public:
    Monster()
    {
        LiveObg();
    }
    Monster(int h, int m, int r, int a, int d, int s, int ab, int x, int y, char *pic, char *equi)
    {
        LiveObg(h, m, r, a, d, s, ab, x, y, pic, equi);
    }
    ~Monster()
    {
		
    }
	
};

class Plant : public LiveObg
{
public:
    Plant();
    ~Plant();
	
};

