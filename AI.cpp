#include <vector>
#include <thread>
#include <array>
#include "AI.h"
#include "constants.h"

// Ϊ����play()�ڼ�ȷ����Ϸ״̬�����£�Ϊ����ֻ��֤��Ϸ״̬�ڵ�����ط���ʱ������
extern const bool asynchronous = false;

// ѡ����Ҫ���ν�player0��player4��ְҵ�����ﶨ��

extern const std::array<THUAI6::StudentType, 4> studentType = {
    THUAI6::StudentType::Athlete,
    THUAI6::StudentType::Teacher,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::Sunshine};

extern const THUAI6::TrickerType trickerType = THUAI6::TrickerType::Assassin;

// ������AI.cpp�ڲ����������뺯?

#pragma region �Զ��帨?
class XY
{
public:
    int x, y;
    XY(int xx, int yy) : x(xx), y(yy) {}
    XY() { x = y = 0; }
};

class XYSquare : public XY
{
public:
    XYSquare(int xx, int yy) : XY(xx, yy) {}
    XYSquare() : XY() {}
};

class XYGrid : public XY
{
public:
    XYGrid(int xx, int yy) : XY(xx, yy)
    {
    }
    XYGrid() : XY()
    {
    }
    XYSquare ToSquare()
    {
        return XYSquare(this->x / myGridPerSquare, this->y / myGridPerSquare);
    }
};

class XYCell : public XY
{
public:
    XYCell(int xx, int yy) : XY(xx, yy)
    {
    }
    XYCell() : XY()
    {
    }
};
int numGridToSquare(int grid) // Converting no. of grids in a given row to its corresponding square number
{
    return grid / myGridPerSquare;
}

XYSquare numGridToXYSquare(int gridx, int gridy) // Converting grid coordinates row to its corresponding square instance
{
    return XYSquare(gridx / myGridPerSquare, gridy / myGridPerSquare);
}

XYCell numSquareToXYCell(int squarex, int squarey)
{
    return XYCell(squarex * myGridPerSquare / numOfGridPerCell, squarey * myGridPerSquare / numOfGridPerCell);
}

#pragma endregion

#pragma region ����
const int myGridPerSquare = 100;
const int myRow = Constants::rows * numOfGridPerCell / myGridPerSquare;
#define framet Constants::frameDuration
#pragma endregion

bool firstTime = false;
std::shared_ptr<const THUAI6::Student> selfInfoStudent;
XYSquare selfSquare;
int myOriVelocity;
int myVelocity;

#pragma region Map
std::vector<std::vector<THUAI6::PlaceType>> oriMap;
int speedOriMap[myRow][myRow];  // 0 Wall
int untilTimeMap[myRow][myRow]; // �����ţ�����У��?
int disMap[myRow][myRow];       // ʵ�ʾ���
XYSquare fromMap[myRow][myRow];

XYCell ClassroomCell[Constants::numOfClassroom];
XYCell OpenedGateCell[2 + 1];
#pragma endregion

void Initializate(const IStudentAPI &api)
{
    if (firstTime)
        return;
    firstTime = true;
    oriMap = api.GetFullMap();
    //    for i in range(myRowAndCol) :
    //       for j in range(myRowAndCol) :
    // speedOriMap[i][j] =
    // find ClassroomSquare
}
void drawMap()
{
    for (int i = 0; i < myRow; ++i)
        for (int j = 0; j < myRow; ++j)
        {
        }
}

int xq[myRow * myRow << 2], yq[myRow * myRow << 2];
int v[myRow][myRow];
inline void SPFAin(int x, int y, int xx, int yy, int vd, int l, int r)
{
    if (untilTimeMap[x][yy] > framet && untilTimeMap[xx][y] > framet)
    {
        if ((disMap[x][y] + vd) * 1000 < myVelocity * ((long long)untilTimeMap[xx][yy] - framet))
            if (disMap[xx][yy] > disMap[x][y] + vd)
            {
                disMap[xx][yy] = disMap[x][y] + vd;
                fromMap[xx][yy] = XYSquare(x, y);
                if (!v[xx][yy])
                    xq[++r] = xx, yq[r] = yy, v[xx][yy] = 1;
            }
    }
}

inline void SPFA()
{
    memset(disMap, 0x3f, sizeof(disMap));
    memset(v, 0, sizeof(v)); // �ڵ���
    int l = 0, r = 1;
    disMap[selfSquare.x][selfSquare.y] = disMap[selfSquare.x][selfSquare.y] = 0;
    v[selfSquare.x][selfSquare.y] = 1;
    fromMap[selfSquare.x][selfSquare.y] = selfSquare;
    xq[1] = selfSquare.x;
    yq[1] = selfSquare.y;
    while (l < r)
    {
        ++l;
        int x = xq[l], y = yq[l];
        v[x][y] = 0;

        int xx = x - 1, yy = y - 1;
        int vd;
        vd = 1 + 1.4142 * myGridPerSquare; //
        SPFAin(x, y, xx, yy, vd, l, r);

        xx += 2;
        SPFAin(x, y, xx, yy, vd, l, r);

        yy += 2;
        SPFAin(x, y, xx, yy, vd, l, r);

        xx -= 2;
        SPFAin(x, y, xx, yy, vd, l, r);

        vd = myGridPerSquare;
        --yy;
        SPFAin(x, y, xx, yy, vd, l, r);

        xx += 2;
        SPFAin(x, y, xx, yy, vd, l, r);

        --xx;
        --yy;
        SPFAin(x, y, xx, yy, vd, l, r);

        yy += 2;
        SPFAin(x, y, xx, yy, vd, l, r);
    }
}

void Move(IAPI &api, XYSquare toMove)
{
    // ��������
}

XYSquare FindMoveNext(XYSquare toMove)
{
    //   return
}

XYSquare FindClassroom()
{
    // û���꣬��?
}

XYSquare FindGate()
{
    // ����У�Ż�У?
    // return (0,0)false
}

bool Commandable()
{
}

void Update(const IStudentAPI &api)
{
    selfInfoStudent = api.GetSelfInfo();
    selfSquare = numGridToXYSquare(selfInfoStudent->x, selfInfoStudent->y);
    // myVelocity
    // OpenedGateCell
}

void Flee(IStudentAPI &api)
{
}

bool TryToLearn(IStudentAPI &api)
{
}

bool TryToGraduate(IStudentAPI &api)
{
}

bool TryToOpenGate(IStudentAPI &api)
{
}

void AI::play(IStudentAPI &api)
{
    Update(api);
    Initializate(api);
    if (Commandable() && !TryToGraduate(api))
    {
        drawMap();
        SPFA();
        if (untilTimeMap[selfSquare.x][selfSquare.y] <= framet)
            Flee(api);
        else
        {
            if (!TryToOpenGate(api))
            {
                XYSquare toGateSquare = FindGate();
                if (toGateSquare.x != 0)
                    Move(api, FindMoveNext(toGateSquare));
                else if (!TryToLearn(api))
                    Move(api, FindMoveNext(FindClassroom()));
            }
        }
    }
    // ��������
    if (this->playerID == 0)
    {
        // ���0ִ�в���
    }
    else if (this->playerID == 1)
    {
        // ���1ִ�в���
    }
    else if (this->playerID == 2)
    {
        // ���2ִ�в���
    }
    else if (this->playerID == 3)
    {
        // ���3ִ�в���
    }
}

void AI::play(ITrickerAPI &api)
{
    auto self = api.GetSelfInfo();
    api.PrintSelfInfo();
}