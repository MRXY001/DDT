#include <Windows.h>
#include <tchar.h>
#include <math.h>
#include <stdio.h>
#include "testOutput.h"
#include "class.h"
// using namespace std;

HINSTANCE hInst;
HWND hWnd;
HDC hdc, mdc, bufdc;
PAINTSTRUCT ps;
RECT rect;
HBITMAP fullmap;

Wind win(WIN_W, WIN_H); // ���ڴ�С����
KeyMouseControler km;   // �������
TurnControler turnc;    // �غϿ���
TimeControler timec;    // ʱ�����
BgMap bgstart, bgmap, bgend;
Role role;
Role role2;
Monster mons;
Animation fly("fly", 50, 200);
Animation bombAttack("bomb", 50, 250);
BombAni bomb;

int tflag = 0, fontWidth = 10;
bool ISEND = false;

void                myInit();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                MyPaint(HDC hdc);

//========== ������ڵ� ==========
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    WNDCLASSEX wcex;

    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = (WNDPROC)WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = NULL;
    wcex.hCursor       = NULL;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = "canvas";
    wcex.hIconSm       = NULL;

    RegisterClassEx(&wcex);

    //��ʼ��
    hInst = hInstance;
    hWnd = CreateWindow("canvas", "������" , WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                        win.w, win.h, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return FALSE;

    MoveWindow(hWnd, 0, 0, win.w, win.h, true);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    hdc = GetDC(hWnd);
    mdc = CreateCompatibleDC(hdc);
    bufdc = CreateCompatibleDC(hdc);
    fullmap = CreateCompatibleBitmap(hdc, win.w, win.h);
    SelectObject(mdc, fullmap);

	timec.setDeadline(1); // �������ڵ�����

    myInit();

    MyPaint(hdc);

    //��Ϣѭ��
    GetMessage(&msg, NULL, (WPARAM)NULL, (LPARAM)NULL); //��ʼ��msg
    while( msg.message != WM_QUIT )
    {
        if( PeekMessage( &msg, NULL, 0, 0 , PM_REMOVE) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            timec.frameNow = GetTickCount();
            if(timec.isNextFrame())
                MyPaint(hdc);
        }
    }

    return msg.wParam;
}

// ========== ��ʼ������ ==========

void myInit()
{
    bgstart.setBg("bg_start");
    srand(GetTickCount() * 10241024);
    switch (rand() % 2)
    {
    case 1 :
        bgmap.setBg("bg_ghost");
        break;
    default :
        bgmap.setBg("bg_rainbow");
    }
    // bgmap.setBg("bg_rainbow");

    bgend.setBg("bg_rainbow");

	//               hp  mp rage  att def spe bomb
    role.setLiveObj(150, 50, 0, 30000, 10, 3, 80000, rand() % (win.w * 2 / 5) + 30, 300, "role", "");
    role.goRight();
    role.setThr("tri_darts");

    role2.setLiveObj(200000, 30, 0, 30, 0, 2, 60, win.w * 3 / 5 + rand() % (win.w * 2/ 5) - 30, 300, "role2", "");
    role2.goLEFT();
    role2.setThr("ice_cream");

    mons.setLiveObj(300, 20, 0, 20, 0, 1, 50, win.w * 4 / 5, 200, "role", "");
}

void startGame() // ��Ϸ��ʼ
{
    turnc.form = FORM_GAME;
    timec.timeStart();
    timec.maxSecond = TURN_DEADLINE;
	timec.nowSecond = timec.nowSecondProc = 0;
    ISEND = false;
    fontWidth = 10;

    myInit();
}

void endGame() // ��Ϸ����
{
    // turnc.form = FORM_END;
    ISEND = true;
}

void prepareAttack()
{
    timec.timePause();              // ��ͣ��ʱ
    turnc.proc = GAME_ACCUMULATING; // ������

    Role *ro;
    if (PVP && turnc.round == ROUND_P2TURN)
        ro = &role2;
    else ro = &role;
    ro->accuming = 0;
}

void startAttack()
{
    if (turnc.proc != GAME_ACCUMULATING) return ;
    turnc.proc = GAME_THROWING;

    Role *ro;
    if (PVP && turnc.round == ROUND_P2TURN)
        ro = &role2;
    else ro = &role;

    int sx, sy, v, vx, vy;
    double an;
    sx = ro->x + ro->w / 2;
    sy = ro->y + ro->h / 2;
    an = ro->angle * PI / 180.0;
    v = ro->accuming;
    if (ro->direct == DRIGHT)
        vx = (int)(v * cos(an));
    else
        vx = -(int)(v * cos(an));
    vy = -(int)(v * sin(an));

    ro->thr.setXYV(sx, sy, vx, vy);
}

bool isInMap(int x, int y) // ���ͼ�����ײ
{
    if (x <= 0 || x >= WIN_W) return true;
    if ( (y >= bgmap.top[x] && y <= bgmap.top[x] + bgmap.height[x])
            || (y >= bgmap.top1[x] && y <= bgmap.top1[x] + bgmap.height1[x])
            || (y >= bgmap.top2[x] && y <= bgmap.top2[x] + bgmap.height2[x])
            || (y >= bgmap.top3[x] && y <= bgmap.top3[x] + bgmap.height3[x]))
    {
        return true;
    }
    return false;
}

bool judgeAttack() // ����������������Ѫ������ͼ�����ײ
{


    if (PVP) // �˶���
    {
        Role *ro/*������*/, *ro2/*��������*/;
        if (turnc.round == ROUND_P2TURN)
        {
            ro = &role2;
            ro2 = &role;
        }
        else
        {
            ro = &role;
            ro2 = &role2;
        }

        if (!(ro->thr.x + ro->thr.w < ro2->x        // left     // �����Է�
                || ro->thr.x > ro2->x + ro2->w      // right
                || ro->thr.y + ro->thr.h < ro2->y   // top
                || ro->thr.y > ro2->y + ro2->h))    // buttom
        {
            if (ro->useFly)
                return true;

            ro2->hpstate = DAMAGE_DEFAULT;

            int attack;
            if (ro->useBomb)
            {
                attack = ro->attack_BOMB * (rand() % 20 + 90) / 100; // һ����Χ�������Ѫ
                ro2->hpstate = DAMAGE_BOW;
            }
            else
                attack = ro->attack * (rand() % 20 + 90) / 100;

            if (rand() % 3 == 0) // ��������
            {
                attack = (int)(attack * 1.5);
                ro2->hpstate += DAMAGE_CRIT;
            }
            else if (rand() % 5 == 0) // �񵲼���
            {
                attack = attack / 2;
                ro2->hpstate = DAMAGE_BLOCK;
            }

            int damage = attack - ro2->defense; // �˺�ͳ��
            if (damage <= 0) damage = 1;
            if (/*ro2->hpstate == DAMAGE_DEFAULT && */rand() % 6 == 0) // ��������
            {
                ro2->cuthp(-damage);
                ro2->hpstate = DAMAGE_IMMU;
            }
            else
            {
                if (ro2->cuthp(damage)) // ��ͨ����
                    endGame();
            }

            if (rand() % 4 == 0) // ���������� bow
            {
                ro2->useBomb = true;
                ro2->setBombThr();
            }

            bomb.addBomb(ro->thr.x + ro->thr.w / 2, ro->thr.y + ro->thr.h / 2);

            return true;
        }
        else if (ro->thr.x + ro->thr.w <= 0 || ro->thr.x >= WIN_W) // ���߽�
        {
            return true;
        }
        else if (isInMap(ro->thr.x, ro->thr.y + ro->thr.h / 2)) // �ϰ���
        {
            bomb.addBomb(ro->thr.x + ro->thr.w / 2, ro->thr.y + ro->thr.h / 2);

            return true;
        }
        else
        {
            return false;
        }
    }
    else // �Ե���
    {
        LiveObg *ro/*������*/, *ro2/*��������*/;
        ro = &role;
        ro2 = &mons;
    }

    return false;
}

void endAttack() // ��������
{
    Role *ro;
    if (turnc.round == ROUND_MYTURN)
        ro = &role;
    else if (turnc.round == ROUND_MONSTER)
        ro = NULL;
    else if (turnc.round == ROUND_P2TURN)
        ro = &role2;

    if (ro != NULL)
    {
        if (ro->useFly) // ֽ�ɻ�����
        {
            int newx = ro->thr.x + ro->thr.w / 2, newy = ro->thr.y + ro->thr.h / 2;
            if (newx <= 0) newx = 1;
            if (newx >= WIN_W) newx = WIN_W - 1;
            if (newy <= 0) newy = 1;
            if (newy >= WIN_H) newy = WIN_H - 1;
            ro->x = newx - ro->w / 2;
            if (ro->thr.vy >= 0)
                ro->y = bgmap.getUpperY(newx, newy) - ro->h;
            else ro->y = bgmap.getLowerY(newx, newy) - ro->h;
            // ro->y = bgmap.getNewY(newx, newy)-ro->h;

            ro->setOThr();
        }
        else if (ro->useBomb)
        {
            ro->setOThr();
        }
        ro->useFly = false;
        ro->useBomb = false;
        ro->roundInit();
    }

    turnc.proc = turnc.next();
}

void throwAttack() // ���﹥������
{
    Role *ro;
    if (PVP && turnc.round == ROUND_P2TURN)
        ro = &role2;
    else ro = &role;

    ro->thr.x += ro->thr.vx;
    ro->thr.y += ro->thr.vy;
    ro->thr.vy += 5; // ��������˵��9.8
    if (ro->thr.x < 0 || ro->thr.x > win.w || ro->thr.y > win.h || judgeAttack() == true)
    {
        ro->accuming = 0;
        endAttack();
    }
}

void RoleControl(UINT message, WPARAM wParam, LPARAM lParam) // ��ɫ���̿���
{
    Role *ro;
    if (PVP && turnc.round == ROUND_P2TURN)
        ro = &role2;
    else ro = &role;

    switch (message)
    {
    case WM_KEYDOWN :
        switch (wParam)
        {
        case VK_UP :
            ro->angle++;
            break;
        case VK_DOWN :
            ro->angle--;
            break;
        case VK_LEFT :
            ro->goLEFT();
            if (ro->mp > 0)
            {
                if (ro->x > 0 && bgmap.canLeft(ro->x, ro->y + ro->h / 2))
                {
                    ro->x -= ro->speed;
                    ro->mp--;
                    ro->y = bgmap.getNewY(ro->x, ro->y);
                }
            }
            break;
        case VK_RIGHT :
            ro->goRight();
            if (ro->mp > 0 && bgmap.canRight(ro->x + ro->w, ro->y + ro->h / 2))
            {
                if (ro->x + ro->w < WIN_W)
                {
                    ro->x += ro->speed;
                    ro->mp--;
                    ro->y = bgmap.getNewY(ro->x, ro->y);

                    /*if (ro->y + ro->h < bgmap.top[ro->x + ro->w / 2])
                        ro->y = bgmap.top[ro->x + ro->w / 2] - ro->h;
                    else if (ro->y + ro->h > bgmap.top[ro->x + ro->w / 2])
                        ro->y = bgmap.top[ro->x + ro->w / 2] - ro->h;*/
                }

            }
            break;
        case 32 :
            ro->accuming++;
            break;
        }
        break;
    case WM_KEYUP :
        switch (wParam)
        {
        case ' ':
            break;
        }
        break;
    }
}

void MonsterControl(UINT message, WPARAM wParam, LPARAM lParam) // �������̿���
{
    switch (message)
    {
    case WM_KEYDOWN :
        ;
    case WM_KEYUP :
        ;
    case WM_CHAR :
        ;
    }
}

void StartControl(UINT message, WPARAM wParam, LPARAM lParam) // ��ʼ���̿���
{
    switch (message)
    {
    case WM_KEYDOWN :
        ;
    case WM_KEYUP :
        ;
    case WM_CHAR :
        ;
    }
}

void EndControl(UINT message, WPARAM wParam, LPARAM lParam) // �������̿���
{
    switch (message)
    {
    case WM_KEYDOWN :
        ;
    case WM_KEYUP :
        ;
    case WM_CHAR :
        ;
    }
}


//========== ��ʱ������ÿ�� 40 ���� ==========
void paintGameForm(HDC hdc)
{
    int i;
    // ===== ������ =====

    bgmap.BitB(mdc, bufdc); // ������

    role.BitB(mdc, bufdc); // ������

    if (PVP) // �Ƿ�����2
    {
        role2.BitB(mdc, bufdc);
    }

    timec.BitB(mdc, bufdc); // ����ʱ��

    // ��������
    int accuml = 0, accumt = WIN_H - 66, accumw = WIN_W, accumh = 60;
    if (turnc.round == ROUND_P2TURN)
    {
        role2.accum.BitB(mdc, accuml, accumt, role2.accuming);
    }
    else
    {
        role.accum.BitB(mdc, accuml, accumt, role.accuming);
    }
    HPEN hp;
    HFONT hf;
    hp = (HPEN) GetStockObject(BLACK_PEN); // Ĭ�ϻ���
    hf = CreateFont(12, 6, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH, "����");
    SelectObject(mdc, hp);
    SelectObject(mdc, hf);
    SetTextColor(mdc, 0x000000/*BGR*/); // ����ɫ
    char str[100];
    for (i = 1; i < 100; i++)
    {
        MoveToEx(mdc, accuml + accumw * i / 100, accumt, NULL);
        LineTo(mdc, accuml + accumw * i / 100, accumt + accumh / 5); // ��ֱ�ߣ���Ҫ���ƶ����λ��
    }
    for (i = 5; i < 100; i += 5)
    {
        MoveToEx(mdc, accuml + accumw * i / 100, accumt, NULL);
        LineTo(mdc, accuml + accumw * i / 100, accumt + accumh / 3); // ��ֱ�ߣ���Ҫ���ƶ����λ��
        sprintf(str, "%d", i);
        TextOut(mdc, accuml + accumw * i / 100 + 2, accumt + accumh / 4, str, strlen(str));
    }
    for (i = 1; i < 10; i++)
    {
        MoveToEx(mdc, accuml + accumw * i / 10, accumt, NULL);
        LineTo(mdc, accuml + accumw * i / 10, WIN_H); // ��ֱ�ߣ���Ҫ���ƶ����λ��
    }
    /* for (i = 0; i < WIN_W; i++)
     {
        MoveToEx(mdc, i, bgmap.top[i], NULL);
        LineTo(mdc, i, bgmap.top[i]+bgmap.height[i]);
        MoveToEx(mdc, i, bgmap.top1[i], NULL);
        LineTo(mdc, i, bgmap.top1[i]+bgmap.height1[i]);
        MoveToEx(mdc, i, bgmap.top2[i], NULL);
        LineTo(mdc, i, bgmap.top2[i]+bgmap.height2[i]);
        MoveToEx(mdc, i, bgmap.top3[i], NULL);
        LineTo(mdc, i, bgmap.top3[i]+bgmap.height3[i]);
     }*/
    DeleteObject(hp); // ������CreatePen��Ҫ�ֶ�ɾ��
    DeleteObject(hf);

    turnc.BitB(mdc);

    if (turnc.proc == GAME_THROWING) // �����У�������
    {
        Role *ro;
        if (PVP && turnc.round == ROUND_P2TURN)
            ro = &role2;
        else ro = &role;
        if (ro->thr.vx < 0)
            ro->thr.BitB(mdc, bufdc);
        else
            ro->thr.BitB_r(mdc, bufdc);

        throwAttack(); // �޸�����λ��
    }
    else if (km.pressing) // �Ƿ�һֱ���Ű���
    {
        RoleControl(WM_KEYDOWN, km.nkey, (LPARAM)NULL);
    }

    Role *ro;
    if (turnc.round == ROUND_MYTURN)
        ro = &role;
    else if (turnc.round == ROUND_P2TURN)
        ro = &role2;
    if (ro != NULL)
    {
        if (ro->useFly)
            fly.BitB1(mdc, bufdc);
        if (ro->useBomb)
            bombAttack.BitB1(mdc, bufdc);
    }

    bomb.BitB(mdc, bufdc);

    if (ISEND) // ������Ϸ
    {
        HFONT hf_end;
        TEXTMETRIC tm;
        if (fontWidth < 30) fontWidth++;
        hf_end = CreateFont(fontWidth << 1, fontWidth, 0, 0, 0, 0, 0, 0, 0, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH, "����"); // (HFONT) GetStockObject(0)

        // �� GameOver
        SelectObject(mdc, hf_end); // �󶨻���
        SetBkMode(mdc, TRANSPARENT);
        SetTextColor(mdc, 0x0000FF/*BGR*/);
        GetTextMetrics(mdc, &tm); // �󶨴�С

        int len = strlen("GameOver") * tm.tmAveCharWidth; // Ҫ���������ܳ���
        TextOut(mdc, win.w / 2 - len / 2, win.h / 2, "GameOver", strlen("GameOver"));

        DeleteObject(hf_end);
    }


    BitBlt(hdc, 0, 0, win.w, win.h, mdc, 0, 0, SRCCOPY); // ����ȥ

    // ===== ���� =====

    if (bgmap.isSuspend(role.x + role.w / 2, role.y + role.h))
        role.y += 10;
    if (PVP) // �жϽ�ɫ 2
    {
        if (bgmap.isSuspend(role2.x + role2.w / 2, role2.y + role2.h))
            role2.y += 10;
    }
    else // �жϹ�����
    {
        ;
    }

    // ===== ��ͼ���� =====

    if (timec.getTimeState() == TIME_OUT) // ÿһ֡
    {
        timec.setDeadline(turnc.next()); // ��ʱ����һ��
        if (turnc.round == ROUND_MYTURN)
            role.roundInit();
        else if (turnc.round == ROUND_MONSTER)
            ;
        else if (turnc.round == ROUND_P2TURN)
            role2.roundInit();
    }


}

void paintStartForm(HDC hdc)
{
    bgstart.BitB(mdc, bufdc);

    timec.BitB(mdc, bufdc);

    BitBlt(hdc, 0, 0, win.w, win.h, mdc, 0, 0, SRCCOPY); // ����ȥ

    if (timec.getTimeState() == TIME_OUT)
    {
        timec.setDeadline(turnc.next());
    }
}

void paintEndForm(HDC hdc)
{
    paintGameForm(hdc);

    // BitBlt(hdc, 0, 0, win.w, win.h, mdc, 0, 0, SRCCOPY); // ����ȥ
}

void MyPaint(HDC hdc)
{
    if (turnc.form == FORM_START)
    {
        paintStartForm(hdc);
    }
    else if (turnc.form == FORM_GAME)
    {
        paintGameForm(hdc);
    }
    else if (turnc.form == FORM_END)
    {
        paintEndForm(hdc);
    }

    timec.updatePre();
}

//========== ��Ϣ������ ==========
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT :
        hdc = BeginPaint(hWnd, &ps);
        GetClientRect(hWnd, &rect);
        MyPaint(hdc);
        EndPaint(hWnd, &ps);
        break;
    case WM_MOUSEMOVE :
        km.x = LOWORD(lParam);
        km.y = HIWORD(lParam);
        break;
    case WM_LBUTTONDOWN :
        km.px = LOWORD(lParam);
        km.py = HIWORD(lParam);
        km.pressing = true;
        break;
    case WM_LBUTTONUP :
        km.px = LOWORD(lParam);
        km.py = HIWORD(lParam);
        km.pressing = false;
        break;
    case WM_KEYDOWN :
        km.nkey = wParam;
        km.pressing = true;

        if (wParam == ' ' && turnc.proc != GAME_ACCUMULATING) // ���¿ո��
        {
            prepareAttack();
        }
        break;
    case WM_KEYUP :
        km.nkey = (LPARAM)NULL;
        km.pressing = false;

        if (wParam == ' ') // ����ո��
        {
            if (ISEND)
            {
                startGame();
            }
            else
            {
                startAttack();
            }

        }
        break;
    case WM_CHAR :
        Role *ro;
        if (turnc.form == FORM_GAME && turnc.round == ROUND_MYTURN)
            ro = &role;
        else if (turnc.form == FORM_GAME && (turnc.round == ROUND_MYTURN || turnc.round == ROUND_P2TURN))
            ro = &role2;
        else
            ro = NULL;

        if (ro != NULL)
        {
            if (wParam == 'f')
            {
                ro->useFly = true;
                ro->setTempThr("flyAttack");
            }
            else if (wParam == 'b'/* && ro->rage >= 100*/)
            {
                /*ro->useBomb = true;
                ro->setTempThr("tri_darts_bomb");*/
            }
        }
        break;
    case WM_DESTROY:                    //���ڽ�����Ϣ����������DC
        DeleteDC(mdc);
        DeleteDC(bufdc);
        DeleteObject(fullmap);
        ReleaseDC(hWnd, hdc);
        PostQuitMessage(0);
        break;
    default:                            //������Ϣ
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
