#include <vector>
#include <thread>
#include<cmath>
#include <array>
#include "AI.h"
#include "constants.h"
using namespace std;

// 为假则play()期间确保游戏状态不更新，为真则只保证游戏状态在调用相关方法时不更新
extern const bool asynchronous = true;

// 选手需要依次将player0到player4的职业在这里定义

extern const std::array<THUAI6::StudentType, 4> studentType = {
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent};

extern const THUAI6::TrickerType trickerType = THUAI6::TrickerType::Assassin;

// 可以在AI.cpp内部声明变量与函数

#pragma region 常量
const int myGridPerSquare = 100;
const int numSquarePerCell = numOfGridPerCell / myGridPerSquare;
const int myRow = Constants::rows * numOfGridPerCell / myGridPerSquare;
const int myCol = Constants::cols * numOfGridPerCell / myGridPerSquare;  // NEW CONSTANT ADDED
#define framet Constants::frameDuration
#pragma endregion

bool firstTime = false;
std::shared_ptr<const THUAI6::Student> selfInfoStudent;
std::shared_ptr<const THUAI6::Tricker> selfInfoTricker;
std::shared_ptr<const THUAI6::Player> selfInfo;
std::shared_ptr<const THUAI6::GameInfo> gameInfo;
std::shared_ptr<const THUAI6::Student> students[4];
int studentsFrame[4];

int myOriVelocity;
int myVelocity;
#pragma region 自定义辅助
class XY {
public:
    int x,y;
    XY(int xx,int yy):x(xx),y(yy){}
    XY()
    {
        x = y = 0;
    }
    XY operator-(const XY& A) const;
    XY operator+(const XY& A) const;
};

XY XY::operator-(const XY& A) const
{
    XY B;
    B.x = this->x - A.x;
    B.y = this->y - A.y;
    return B;
}
XY XY::operator+(const XY& A) const
{
    XY B;
    B.x = this->x + A.x;
    B.y = this->y + A.y;
    return B;
}


class XYSquare :public XY {
public:
    XYSquare(int xx, int yy) :
        XY(xx, yy){}
    XYSquare() :
        XY(){}
}selfSquare;

class XYGrid : public XY
{
public:
    XYGrid(int xx, int yy) :
        XY(xx, yy)
    {
    }
    XYGrid() :
        XY()
    {
    }
};
XYSquare GridToSquare(XYGrid xy)
    {
        return XYSquare(xy.x / myGridPerSquare, xy.y / myGridPerSquare);
    }

    XYGrid SquareToGrid(XYSquare xy)
    {
        return XYGrid(xy.x * myGridPerSquare + myGridPerSquare / 2, xy.y * myGridPerSquare + myGridPerSquare / 2);
    }

    class XYCell : public XY
{
public:
    XYCell(int xx, int yy) :
        XY(xx, yy)
    {
    }
    XYCell() :
        XY()
    {
    }
};

    XYGrid CellToGrid(XYCell xy)
    {
        return XYGrid(xy.x * numOfGridPerCell + numOfGridPerCell / 2, xy.y * numOfGridPerCell + numOfGridPerCell / 2);
    }
    XYCell numGridToXYCell(int x, int y)
    {
        return XYCell(x / numOfGridPerCell, y / numOfGridPerCell);
    }
    XYCell XYToCell(XY xy)
    {
        return XYCell(xy.x,xy.y);
    }
    int numGridToSquare(int grid)  // Converting no. of grids in a given row to its corresponding square number
{
    return grid / myGridPerSquare;
}

int numGridToCell(int grid)  
{
    return grid / numOfGridPerCell;
}

XYSquare numGridToXYSquare(int gridx, int gridy)  // Converting grid coordinates row to its corresponding square instance
{
    return XYSquare(gridx / myGridPerSquare, gridy / myGridPerSquare);
}

XYCell numSquareToXYCell(int squarex, int squarey)
{
    return XYCell(squarex /numSquarePerCell, squarey /numSquarePerCell);
}

bool IsApproach(XYCell cell)
{
    return abs(numGridToCell(selfInfo->x) - cell.x) <= 1 && abs(numGridToCell(selfInfo->y) - cell.y) <= 1;
}
bool ApproachToInteractInACross(XYCell cell)
{
    return (abs(numGridToCell(selfInfo->x) - cell.x) + abs(numGridToCell(selfInfo->y) - cell.y)) <= 1;
}
XYCell SquareToCell(XYSquare xy)
{
    return XYCell(xy.x / numSquarePerCell, xy.y / numSquarePerCell);
}
int DistanceUP(int x, int y)
{
    return sqrt((x - selfInfo->x) * (x - selfInfo->x) + (y - selfInfo->y) * (y - selfInfo->y))+1;
}
#pragma endregion

#pragma region Map
std::vector<std::vector<THUAI6::PlaceType>> oriMap;
int speedOriMap[myRow][myRow];//0 Wall
int untilTimeMap[myRow][myRow];//处理门，隐藏校门等
int disMap[myRow][myRow];//实际距离
XYSquare fromMap[myRow][myRow];

XYCell ClassroomCell[Constants::numOfClassroom];
XYCell openedGateCell[2+1];
XYCell GateCell[2];
//XYCell DoorCell
#pragma endregion

 THUAI6::PlaceType CellPlaceType(XYCell cell)
{
    return oriMap[cell.x][cell.y];
 }

void Initialize(const IStudentAPI& api)
{
    if (firstTime)
        return;
    memset(speedOriMap, 0x3f, sizeof(speedOriMap));
    firstTime = true;
    oriMap = api.GetFullMap();
    myOriVelocity = api.GetSelfInfo()->speed;
    int i = 0, j = 0, numClassroom= 0,numGate=0;  
    int l = 0;                   // temp

    for (i = 0; i < Constants::rows; ++i)  // traverse all Cells
    {
        for (j = 0; j < Constants::rows; ++j)
        {
            switch (oriMap[i][j])  // To seek the placetype of this square, convert coordinates to cells first
            {
                case THUAI6::PlaceType::Wall:
                case THUAI6::PlaceType::NullPlaceType:
                case THUAI6::PlaceType::Chest:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2+1, 0); ii < min((i + 1) * numSquarePerCell + numSquarePerCell / 2-1, 490);++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < min((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 490); ++jj)
                            speedOriMap[ii][jj] = 0;
                    break;
                case THUAI6::PlaceType::Gate:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = 0;
                    GateCell[numGate++] = XYCell(i, j);
                    break;
                case THUAI6::PlaceType::ClassRoom:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = 0;
                    ClassroomCell[numClassroom++] = XYCell(i, j);
                    break;
                case THUAI6::PlaceType::Window:  // Cases with windows differ with player type and is non-linear with myOriVelocity
                    switch (api.GetSelfInfo()->studentType)
                    {
                        case THUAI6::StudentType::Athlete:
                            l = Constants::Athlete::speedOfClimbingThroughWindows;
                            break;
                        case THUAI6::StudentType::Teacher:
                            l = Constants::Teacher::speedOfClimbingThroughWindows;
                            break;
                        case THUAI6::StudentType::StraightAStudent:
                            l = Constants::StraightAStudent::speedOfClimbingThroughWindows;
                            break;
                        case THUAI6::StudentType::Sunshine:
                            l = Constants::Sunshine::speedOfClimbingThroughWindows;
                            break;
                        default:
                            l = 0;
                            break;
                    }
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = l;
                    break;
                case THUAI6::PlaceType::Land:
                case THUAI6::PlaceType::Grass:
                default:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = myOriVelocity;
                    break;
            }
        }
    }
}

void Initialize(const ITrickerAPI& api)
{
    if (firstTime)
        return;
    memset(speedOriMap, 0x3f, sizeof(speedOriMap));
    firstTime = true;
    oriMap = api.GetFullMap();
    myOriVelocity = api.GetSelfInfo()->speed;
    int i = 0, j = 0, numClassroom = 0, numGate = 0;
    int l = 0;  // temp

    for (i = 0; i < Constants::rows; ++i)  // traverse all Cells
    {
        for (j = 0; j < Constants::rows; ++j)
        {
            switch (oriMap[i][j])  // To seek the placetype of this square, convert coordinates to cells first
            {
                case THUAI6::PlaceType::Wall:
                case THUAI6::PlaceType::NullPlaceType:
                case THUAI6::PlaceType::Chest:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < min((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 490); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < min((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 490); ++jj)
                            speedOriMap[ii][jj] = 0;
                    break;
                case THUAI6::PlaceType::Gate:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = 0;
                    GateCell[numGate++] = XYCell(i, j);
                    break;
                case THUAI6::PlaceType::ClassRoom:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = 0;
                    ClassroomCell[numClassroom++] = XYCell(i, j);
                    break;
                case THUAI6::PlaceType::Window:  // Cases with windows differ with player type and is non-linear with myOriVelocity
                    l = Constants::Assassin::speedOfClimbingThroughWindows;
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = l;
                    break;
                case THUAI6::PlaceType::Land:
                case THUAI6::PlaceType::Grass:
                default:
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            speedOriMap[ii][jj] = myOriVelocity;
                    break;
            }
        }
    }
}
void drawMap(const ITrickerAPI& api)
{
    memset(untilTimeMap, 0x3f, sizeof(untilTimeMap));
    for (int i = 0; i < Constants::rows; ++i)  // traverse all Cells
    {
        for (int j = 0; j < Constants::rows; ++j)
        {
            switch (oriMap[i][j])  // To seek the placetype of this square, convert coordinates to cells first
            {
                case THUAI6::PlaceType::Wall:
                case THUAI6::PlaceType::NullPlaceType:
                case THUAI6::PlaceType::ClassRoom:
                case THUAI6::PlaceType::Gate:
                case THUAI6::PlaceType::Chest:
                    //
                case THUAI6::PlaceType::Door6:
                case THUAI6::PlaceType::Door3:
                case THUAI6::PlaceType::Door5:
                case THUAI6::PlaceType::HiddenGate:
                    //   for (int k=)
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2 + 1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2 - 1, 0); ++jj)
                            untilTimeMap[ii][jj] = -1;
                    break;
                default:
                    break;
            }
        }
    }
}

void drawMap(const IStudentAPI& api)
{
    memset(untilTimeMap, 0x3f, sizeof(untilTimeMap));
    for (int i = 0; i < Constants::rows; ++i)  // traverse all Cells
    {
        for (int j = 0; j < Constants::rows; ++j)
        {
            switch (oriMap[i][j])  // To seek the placetype of this square, convert coordinates to cells first
            {
                case THUAI6::PlaceType::Wall:
                case THUAI6::PlaceType::NullPlaceType:
                case THUAI6::PlaceType::ClassRoom:
                case THUAI6::PlaceType::Gate:
                case THUAI6::PlaceType::Chest:
                    //
                case THUAI6::PlaceType::Door6:
                case THUAI6::PlaceType::Door3:
                case THUAI6::PlaceType::Door5:
                case THUAI6::PlaceType::HiddenGate:
                    //   for (int k=)
                    for (int ii = max((i - 1) * numSquarePerCell + numSquarePerCell / 2+1, 0); ii < max((i + 1) * numSquarePerCell + numSquarePerCell / 2-1, 0); ++ii)
                        for (int jj = max((j - 1) * numSquarePerCell + numSquarePerCell / 2+1, 0); jj < max((j + 1) * numSquarePerCell + numSquarePerCell / 2-1, 0); ++jj)
                            untilTimeMap[ii][jj] = -1;
                    break;
                default:
                    break;
            }
        }
    }
     auto t=api.GetTrickers();
    if (t.size())
    {
        int tx =numGridToSquare(t[0]->x), ty = numGridToSquare(t[0]->y);
        for (int i = max(tx - 20, 5); i < min(tx + 20, 496); ++i)
            for (int j = max(ty - 20, 5); j < min(ty + 20, 496); ++j)
                untilTimeMap[i][j] = -1;
    }
    auto tl = api.GetStudents();
    for (int n = 0; n < 4; ++n)
        if (tl[n]->playerID!=selfInfo->playerID)
    {
        int tx = numGridToSquare(tl[n]->x), ty = numGridToSquare(tl[n]->y);
        for (int i = max(tx - 10, 5); i < min(tx + 10, 496); ++i)
            for (int j = max(ty - 10, 5); j < min(ty + 10, 496); ++j)
                untilTimeMap[i][j] = -1;
    }
}

int xq[myRow * myRow << 2], yq[myRow * myRow << 2];
int v[myRow][myRow];
inline void SPFAin(int x,int y,int xx,int yy,int vd,int l,int& r)
{
    if (untilTimeMap[x][yy] > framet && untilTimeMap[xx][y] > framet)
    {
    //printf("%d %d %d %d SPFA\n", x, y, xx, yy);
        if ((disMap[x][y] + vd) * 1000 < myVelocity * ((long long)untilTimeMap[xx][yy] - framet))
            if (disMap[xx][yy] > disMap[x][y] + vd)
            {
      //        if(xx==225)  printf("%d %d\n",xx,yy);
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
    memset(v, 0, sizeof(v));  // 节点标记
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
        vd = (1 + 1.4142 * myGridPerSquare) * (speedOriMap[x][y] + speedOriMap[xx][yy])/(2*myOriVelocity);  
        SPFAin(x, y, xx, yy, vd,l,r);

        xx += 2;
        vd = (1 + 1.4142 * myGridPerSquare) * (speedOriMap[x][y] + speedOriMap[xx][yy]) / (2 * myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);

        yy += 2;
        vd = (1 + 1.4142 * myGridPerSquare) * (speedOriMap[x][y] + speedOriMap[xx][yy]) / (2 * myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);

        xx -= 2;
        vd = (1 + 1.4142 * myGridPerSquare) * (speedOriMap[x][y] + speedOriMap[xx][yy]) / (2 * myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);

        --yy;
        vd = myGridPerSquare * (speedOriMap[x][y] + speedOriMap[xx][yy])/(2*myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);

        xx += 2;
        vd = myGridPerSquare * (speedOriMap[x][y] + speedOriMap[xx][yy]) / (2 * myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);

        --xx;
        --yy;
        vd = myGridPerSquare * (speedOriMap[x][y] + speedOriMap[xx][yy]) / (2 * myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);

        yy += 2;
        vd = myGridPerSquare * (speedOriMap[x][y] + speedOriMap[xx][yy]) / (2 * myOriVelocity);  
        SPFAin(x, y, xx, yy, vd, l, r);
    }
}

void Move(IAPI& api, XYSquare toMove)
{
    // 包括翻窗
    api.EndAllAction();
    XYCell cell = XYToCell(SquareToCell(selfSquare) + (toMove - selfSquare));
    if (CellPlaceType(cell) == THUAI6::PlaceType::Window && ApproachToInteractInACross(cell))
    {
        api.SkipWindow();
        return;
    }
    int x = SquareToGrid(toMove).x;
    int y = SquareToGrid(toMove).y;
    api.Move(60, atan2(y - selfInfo->y, x - selfInfo->x));
}

XYSquare FindMoveNext(XYSquare toMove)
{
   // printf("%d %d MoveNext\n", toMove.x, toMove.y);
    int x = toMove.x, y = toMove.y, lx = toMove.x, ly = toMove.x;
    while (x != selfSquare.x || y != selfSquare.y)
    {
        lx = x;
        ly = y;
        x = fromMap[lx][ly].x;
        y = fromMap[lx][ly].y;
    }
    return XYSquare(lx, ly);
}

XYSquare FindNearInCell(XYCell cell)
{
    int i = max((cell.x-1) * numSquarePerCell, 10);
    int j = max((cell.y - 1) * numSquarePerCell, 10);
    int d = disMap[i][j];
    XYSquare toFind = XYSquare(i,j);
    for (; j <min(490,(cell.y+2) * numSquarePerCell);++j)
        if (disMap[i][j]<d)
        {
            d = disMap[i][j];
            toFind = XYSquare(i, j);
        }
    --j;
    for (; i < min(490, (cell.x + 2) * numSquarePerCell); ++i)
        if (disMap[i][j] < d)
        {
            d = disMap[i][j];
            toFind = XYSquare(i, j);
        }
    --i;
    for (; j >= max((cell.y - 1) * numSquarePerCell, 10); --j)
        if (disMap[i][j] < d)
        {
            d = disMap[i][j];
            toFind = XYSquare(i, j);
        }
    ++j;
    for (; i >= max((cell.x - 1) * numSquarePerCell, 10); --i)
        if (disMap[i][j] < d)
        {
            d = disMap[i][j];
            toFind = XYSquare(i, j);
        }
    return toFind;
}

XYSquare FindClassroom()
{
    XYSquare toFind = FindNearInCell(ClassroomCell[0]);
    int d = disMap[toFind.x][toFind.y];
    //printf("%d %d %d Class\n", toFind.x, toFind.y, disMap[toFind.x][toFind.y]);
    for (int i = 1; i < Constants::numOfClassroom; ++i)
    {
        XYSquare t = FindNearInCell(ClassroomCell[i]);
        if (disMap[t.x][t.y] < d)
        {
            toFind = t;
            d = disMap[toFind.x][toFind.y];
        }
       // printf("%d %d %d %d Class\n", t.x, t.y, i, disMap[t.x][t.y]);
    }
    return toFind;
}

XYSquare FindGate()
{
    //if (gameInfo->studentGraduated + gameInfo->studentQuited==3&& gameInfo->subjectFinished >= Constants::numOfRequiredClassroomForHiddenGate)  
    if (gameInfo->subjectFinished < Constants::numOfRequiredClassroomForGate)
        return XYSquare(0, 0);

    XYSquare toFind = FindNearInCell(GateCell[0]);
    int d = disMap[toFind.x][toFind.y];

        XYSquare t = FindNearInCell(GateCell[1]);
        if (disMap[t.x][t.y] < d)
        {
            toFind = t;
            d = disMap[toFind.x][toFind.y];
        }
    return toFind;
    //隐藏校门或校门
    //return (0,0)false
}

bool Commandable(const IAPI& api)
{
    auto p = selfInfo->playerState;
  //  printf("ok\n");
 //   api.PrintSelfInfo();
    return p != THUAI6::PlayerState::Addicted && p != THUAI6::PlayerState::Roused && p != THUAI6::PlayerState::Climbing && p != THUAI6::PlayerState::Attacking && p != THUAI6::PlayerState::Graduated && p != THUAI6::PlayerState::Quit && p != THUAI6::PlayerState::Stunned && p != THUAI6::PlayerState::Swinging;
}

void Update(const IStudentAPI& api)
{
    gameInfo = api.GetGameInfo();
    selfInfoStudent = api.GetSelfInfo();
    selfInfo = selfInfoStudent;
    selfSquare = numGridToXYSquare(selfInfoStudent->x, selfInfoStudent->y);
    //printf("%d %d self\n", selfSquare.x, selfSquare.y);
    for (int i = 0; i < Constants::numOfClassroom; ++i)
        if (api.GetClassroomProgress(ClassroomCell[i].x, ClassroomCell[i].y) == 10000000)
            ClassroomCell[i] = XYCell(0, 0);
    myVelocity = selfInfo->speed;
    //openedGateCell
    for (int i = 0; i < 2; ++i)
        if (api.GetGateProgress(ClassroomCell[i].x, ClassroomCell[i].y) == 18000)
            openedGateCell[i] = GateCell[i];
}

void Update(const ITrickerAPI& api)
{
    gameInfo = api.GetGameInfo();
    selfInfoTricker = api.GetSelfInfo();
    selfInfo = selfInfoTricker;
    selfSquare = numGridToXYSquare(selfInfoTricker->x, selfInfoTricker->y);
    for (int i = 0; i < Constants::numOfClassroom; ++i)
    {
        if (api.GetClassroomProgress(ClassroomCell[i].x, ClassroomCell[i].y) == 10000000)
            ClassroomCell[i] = XYCell(0, 0);
    }
    myVelocity = selfInfo->speed;
    auto stu = api.GetStudents();
    if (stu.size()>0)
    {
        for (int i = 0; i < stu.size(); ++i)
            students[stu[i]->playerID] = stu[i], studentsFrame[stu[i]->playerID] = api.GetFrameCount();
    }
    // openedGateCell
    for (int i = 0; i < 2; ++i)
        if (api.GetGateProgress(ClassroomCell[i].x, ClassroomCell[i].y) == 18000)
            openedGateCell[i] = GateCell[i];
}

void Flee(IStudentAPI& api)
{
}

bool TryToLearn(IStudentAPI& api)
{
        for (int i = 0; i < Constants::numOfClassroom; ++i)
        if (ClassroomCell[i].x != 0 && IsApproach(ClassroomCell[i]))
        {
            if (selfInfo->timeUntilSkillAvailable[0] == 0&&selfInfo->playerState!=THUAI6::PlayerState::Learning)
                api.UseSkill(0);
            api.StartLearning();
            return true;
        }
        return false;
}

bool TryToGraduate(IStudentAPI& api)
{
    return api.Graduate().get();
}

bool TryToOpenGate(IStudentAPI& api)
{
        for (int i = 0; i < 2; ++i)
        if (GateCell[i].x != 0 && IsApproach(GateCell[i]))
        {
            //if (selfInfo->)
            api.StartOpenGate();
            printf("OK!");
            return true;
        }
        return false;
}

void AI::play(IStudentAPI& api)
{
    Update(api);
    Initialize(api);
    if (Commandable(api) && !TryToGraduate(api))
    {
        drawMap(api);
        SPFA();
//        if (untilTimeMap[selfSquare.x][selfSquare.y] <= framet)Flee(api);
  //      else
    //    {
        if (!TryToOpenGate(api))
        {
            XYSquare toGateSquare = FindGate();
        //    api.Print("!!!\n");
            if (toGateSquare.x != 0)
            {
                Move(api, FindMoveNext(toGateSquare));
                // api.Print("!!!\n");
            }
            else
            {
                if (!TryToLearn(api))
                {
                //    api.Print("!!!!!\n");
                    Move(api, FindMoveNext(FindClassroom()));
                }
            }
        }
      //  }
    }
    // 公共操作
    if (this->playerID == 0)
    {
        // 玩家0执行操作
    }
    else if (this->playerID == 1)
    {
        // 玩家1执行操作
    }
    else if (this->playerID == 2)
    {
        // 玩家2执行操作
    }
    else if (this->playerID == 3)
    {
        // 玩家3执行操作
    }
}

XYSquare FindStudent()
{
    int i = 0;
    while (students[i] == nullptr&&i<4)
        ++i;
    if (i == 4)
        return XYSquare(0, 0);
    XYSquare toFind = FindNearInCell(numGridToXYCell(students[i]->x, students[i]->y));
    int d = disMap[toFind.x][toFind.y];
    // printf("%d %d %d Class\n", toFind.x, toFind.y, disMap[toFind.x][toFind.y]);
    for (; i < 4; ++i)
        if (students[i] != nullptr && students[i]->playerState != THUAI6::PlayerState::Addicted && students[i]->playerState != THUAI6::PlayerState::Roused)
    {
            XYSquare t = FindNearInCell(numGridToXYCell(students[i]->x, students[i]->y));
        if (disMap[t.x][t.y] < d)
        {
            toFind = t;
            d = disMap[toFind.x][toFind.y];
        }
    }
    return toFind;
}

bool TryToAttack(ITrickerAPI& api)
{
    int attackrange = (selfInfo->bulletType == THUAI6::BulletType::CommonAttackOfTricker) ? Constants::CommonAttackOfTricker::BulletAttackRange : Constants::FlyingKnife::BulletAttackRange;
    if (api.GetStudents().size() > 0)
    {
    for (int i = 0; i < api.GetStudents().size(); ++i)
        if (api.GetStudents()[i]->playerState != THUAI6::PlayerState::Addicted && api.GetStudents()[i]->playerState != THUAI6::PlayerState::Roused)
        {
            if (selfInfo->timeUntilSkillAvailable[0] == 0)
                    api.UseSkill(0);
            auto st = api.GetStudents()[i];
            if (DistanceUP(st->x, st->y) < attackrange)
            {
                    api.Attack(atan2(st->y - selfInfo->y, st->x - selfInfo->x));
                    return true;
            }
            else
            {
                    if (selfInfo->timeUntilSkillAvailable[1] == 0)
                    {
                        api.UseSkill(1);
                        api.Attack(atan2(st->y - selfInfo->y, st->x - selfInfo->x));
                        return true;
                    }
                    else
                        Move(api, FindMoveNext(FindNearInCell(numGridToXYCell(st->x, st->y))));
            }
            return true;
        }
    return false;
    }
    else
    return false;
}

void AI::play(ITrickerAPI& api)
{
      // api.Print("?\n");
    Update(api);
    Initialize(api);
     api.Print("!\n");
    drawMap(api);
    SPFA();
    if (Commandable(api) && !TryToAttack(api))
    {
    //XYSquare toGateSquare = FindStudent();
    //if (toGateSquare.x != 0)
    //{
     // api.Print("!!\n");
      //  Move(api, FindMoveNext(toGateSquare));
   // }
    //else
    //{
        XYSquare toGateSquare = FindGate();
        if (toGateSquare.x != 0)
        {
            Move(api, FindMoveNext(toGateSquare));
        }
        else
        {
            Move(api, FindMoveNext(FindClassroom()));
        }
    }
    //}
}