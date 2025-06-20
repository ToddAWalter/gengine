#include "SidneyAnalyze.h"

#include "Actor.h"
#include "ActionManager.h"
#include "AssetManager.h"
#include "AudioManager.h"
#include "Color32.h"
#include "CursorManager.h"
#include "GameProgress.h"
#include "InputManager.h"
#include "LocationManager.h"
#include "RectTransform.h"
#include "Sidney.h"
#include "SidneyFakeInputPopup.h"
#include "SidneyFiles.h"
#include "UIButton.h"
#include "UICanvas.h"
#include "UICircles.h"
#include "UIGrids.h"
#include "UIHexagrams.h"
#include "UIImage.h"
#include "UILabel.h"
#include "UILines.h"
#include "UIPoints.h"
#include "UIRectangles.h"
#include "UIUtil.h"

namespace
{
    // Map sizes, in pixels.
    const float kZoomedOutMapSize = 342.0f;
    const float kZoomedInMapSize = 1368.0f;

    // Zoomed in map view size, in pixels.
    const float kZoomedInMapViewWidth = 271.0f;
    const float kZoomedInMapViewHeight = 324.0f;

    // The distance from the mouse pointer to a point for that point to be considered "hovered."
    const float kHoverPointDist = 4.0f;
    const float kHoverPointDistSq = kHoverPointDist * kHoverPointDist;

    // The distance from a placed point to a solution point that is considered "close enough" to trigger the solution.
    // For example, you must place 2 points to solve Aquarius, but the placed points don't need to be exact - just close enough.
    const float kCloseToPointDist = 20.0f;
    const float kCloseToPointDistSq = kCloseToPointDist * kCloseToPointDist;

    // The colors of unselected shapes in both views.
    const Color32 kZoomedOutShapeColor(0, 255, 0, 255);
    const Color32 kZoomedInShapeColor(0, 255, 0, 192);

    // The color of selected shapes changes in the zoomed out view only.
    const Color32 kZoomedOutSelectedShapeColor(0, 0, 0, 255);

    // The colors of locked grids in both views.
    const Color32 kZoomedInLockedGridColor(128, 128, 128, 192);
    const Color32 kZoomedOutLockedGridColor(128, 128, 128, 255);

    // The colors of locked shapes in both views.
    const Color32 kZoomedInLockedShapeColor(0, 0, 255, 192);
    const Color32 kZoomedOutLockedShapeColor(0, 0, 255, 255);

    // The colors of lines in both views.
    const Color32 kZoomedInLineColor(0, 0, 0, 192);
    const Color32 kZoomedOutLineColor(0, 0, 0, 255);

    // Specific points required for various parts of the LSR map puzle.
    const Vector2 kPiscesCoustaussaPoint(405.0f, 1094.0f);
    const Vector2 kPiscesBezuPoint(301.0f, 385.0f);
    const Vector2 kPiscesBugarachPoint(991.0f, 327.0f);

    const Vector2 kTaurusSerresPoint(815.0f, 1167.0f);
    const Vector2 kTaurusMeridianPoint(898.0f, 1128.0f);

    bool IsPointOnEdgeOfRectangle(const UIRectangle& rect, const Vector2& point)
    {
        // Convert point to local space of the rectangle.
        // After doing this, we can just worry about solving as though the rectangle has no rotation.
        Vector3 localPoint = Matrix3::MakeRotateZ(-rect.angle).TransformPoint(point - rect.center);

        // Calculate half width/height of the rect.
        float halfWidth = rect.size.x * 0.5f;
        float halfHeight = rect.size.y * 0.5f;

        // Get how far the point is from the center on both axes.
        float distFromCenterX = Math::Abs(localPoint.x);
        float distFromCenterY = Math::Abs(localPoint.y);

        // Determine whether the point was near any of the edges of the rectangle.
        const float kCloseToEdgeDist = 2.5f;
        bool nearLeftRightSides = Math::Abs(halfWidth - distFromCenterX) < kCloseToEdgeDist;
        bool nearTopBottomSides = Math::Abs(halfHeight - distFromCenterY) < kCloseToEdgeDist;

        // To actually be on the edge, one of those two bools must be true...
        // ...AND the distance on the opposite axis must be within the half-size of the shape.
        return (nearLeftRightSides && distFromCenterY <= halfHeight) || (nearTopBottomSides && distFromCenterX <= halfWidth);
    }

    bool IsPointInsideRectangle(const UIRectangle& rect, const Vector2& point)
    {
        // Convert point to local space of the rectangle.
        // After doing this, we can just worry about solving as though the rectangle has no rotation.
        Vector3 localPoint = Matrix3::MakeRotateZ(-rect.angle).TransformPoint(point - rect.center);

        // Calculate half width/height of the rect.
        float halfWidth = rect.size.x * 0.5f;
        float halfHeight = rect.size.y * 0.5f;

        // Get how far the point is from the center on both axes.
        float distFromCenterX = Math::Abs(localPoint.x);
        float distFromCenterY = Math::Abs(localPoint.y);

        // Inside the rectangle if within half sizes on both axes.
        return distFromCenterX < halfWidth && distFromCenterY < halfHeight;
    }

    bool IsPointOnEdgeOfHexagram(const UIHexagram& hexagram, const Vector2& point)
    {
        // Convert point to local space of the hexagram.
        // After doing this, we can just worry about solving as though the hexagram has no rotation.
        Vector3 localPoint = Matrix3::MakeRotateZ(-hexagram.angle).TransformPoint(point - hexagram.center);

        // Get six hexagram points.
        const float kAngleInterval = Math::k2Pi / 6;
        Vector3 points[6];
        for(int j = 0; j < 6; ++j)
        {
            float angle = j * kAngleInterval;
            points[j].x = hexagram.radius * Math::Sin(angle);
            points[j].y = hexagram.radius * Math::Cos(angle);
        }

        // Generate line segments for each one.
        LineSegment segments[6] = {
            LineSegment(points[0], points[2]),
            LineSegment(points[2], points[4]),
            LineSegment(points[4], points[0]),
            LineSegment(points[1], points[3]),
            LineSegment(points[3], points[5]),
            LineSegment(points[5], points[1])
        };
        for(int i = 0; i < 6; ++i)
        {
            Vector2 closestPoint = segments[i].GetClosestPoint(localPoint);
            float length = (closestPoint - localPoint).GetLengthSq();
            if(length < 16)
            {
                return true;
            }
        }
        return false;
    }
}

Vector2 SidneyAnalyze::MapState::View::GetLocalMousePos()
{
    // Subtract min from mouse pos to get point relative to lower left corner.
    return gInputManager.GetMousePosition() - mapImage->GetRectTransform()->GetWorldRect().GetMin();
}

Vector2 SidneyAnalyze::MapState::View::GetPlacedPointNearPoint(const Vector2& point, bool useLockedPoints)
{
    // Iterate through points placed by the user.
    // Find one that is close enough to the desired point and return it.
    UIPoints* pts = useLockedPoints ? lockedPoints : points;
    for(size_t i = 0; i < pts->GetPointsCount(); ++i)
    {
        const Vector2& placedPoint = pts->GetPoint(i);
        if((placedPoint - point).GetLengthSq() < kCloseToPointDistSq)
        {
            return placedPoint;
        }
    }

    // No point placed by user is near the passed in point - return Zero as a placeholder/default.
    return Vector2::Zero;
}

void SidneyAnalyze::MapState::View::OnPersist(PersistState& ps)
{
    // We *could* add OnPersist methods to the various UI classes, so we can save points/lines/etc.
    // However, I'm unsure if I want UI code to be closely coupled to the persistence code.
    // For now, I'll just manually save/load these UI classes here.

    // Points
    {
        std::vector<Vector2> xferPoints;
        if(ps.IsSaving())
        {
            for(int i = 0; i < points->GetPointsCount(); ++i)
            {
                xferPoints.push_back(points->GetPoint(i));
            }
        }
        ps.Xfer("points", xferPoints);
        if(ps.IsLoading())
        {
            points->ClearPoints();
            for(Vector2& p : xferPoints)
            {
                points->AddPoint(p);
            }
        }
    }

    // Locked Points
    {
        std::vector<Vector2> xferPoints;
        if(ps.IsSaving())
        {
            for(int i = 0; i < lockedPoints->GetPointsCount(); ++i)
            {
                xferPoints.push_back(lockedPoints->GetPoint(i));
            }
        }
        ps.Xfer("lockedPoints", xferPoints);
        if(ps.IsLoading())
        {
            lockedPoints->ClearPoints();
            for(Vector2& p : xferPoints)
            {
                lockedPoints->AddPoint(p);
            }
        }
    }

    // Lines
    {
        std::vector<LineSegment> xferLines;
        if(ps.IsSaving())
        {
            for(int i = 0; i < lines->GetLinesCount(); ++i)
            {
                xferLines.push_back(lines->GetLine(i));
            }
        }
        ps.Xfer("lines", xferLines);
        if(ps.IsLoading())
        {
            lines->ClearLines();
            for(LineSegment& ls : xferLines)
            {
                lines->AddLine(ls.start, ls.end);
            }
        }
    }

    // Circles
    {
        std::vector<Circle> xfer;
        if(ps.IsSaving())
        {
            for(int i = 0; i < circles->GetCirclesCount(); ++i)
            {
                xfer.push_back(circles->GetCircle(i));
            }
        }
        ps.Xfer("circles", xfer);
        if(ps.IsLoading())
        {
            circles->ClearCircles();
            for(Circle& circle : xfer)
            {
                circles->AddCircle(circle.center, circle.radius);
            }
        }
    }

    // Locked Circles
    {
        std::vector<Circle> xfer;
        if(ps.IsSaving())
        {
            for(int i = 0; i < lockedCircles->GetCirclesCount(); ++i)
            {
                xfer.push_back(lockedCircles->GetCircle(i));
            }
        }
        ps.Xfer("lockedCircles", xfer);
        if(ps.IsLoading())
        {
            lockedCircles->ClearCircles();
            for(Circle& circle : xfer)
            {
                lockedCircles->AddCircle(circle.center, circle.radius);
            }
        }
    }

    // Rectangles
    {
        std::vector<UIRectangle> xfer;
        if(ps.IsSaving())
        {
            for(int i = 0; i < rectangles->GetCount(); ++i)
            {
                xfer.push_back(rectangles->GetRectangle(i));
            }
        }
        ps.Xfer("rectangles", xfer);
        if(ps.IsLoading())
        {
            rectangles->ClearRectangles();
            for(UIRectangle& rect : xfer)
            {
                rectangles->AddRectangle(rect.center, rect.size, rect.angle);
            }
        }
    }

    // Locked Rectangles
    {
        std::vector<UIRectangle> xfer;
        if(ps.IsSaving())
        {
            for(int i = 0; i < lockedRectangles->GetCount(); ++i)
            {
                xfer.push_back(lockedRectangles->GetRectangle(i));
            }
        }
        ps.Xfer("lockedRectangles", xfer);
        if(ps.IsLoading())
        {
            lockedRectangles->ClearRectangles();
            for(UIRectangle& rect : xfer)
            {
                lockedRectangles->AddRectangle(rect.center, rect.size, rect.angle);
            }
        }
    }

    // Grids
    {
        std::vector<UIGrid> xfer;
        if(ps.IsSaving())
        {
            for(int i = 0; i < grids->GetCount(); ++i)
            {
                xfer.push_back(grids->GetGrid(i));
            }
        }
        ps.Xfer("grids", xfer);
        if(ps.IsLoading())
        {
            grids->Clear();
            for(UIGrid& grid : xfer)
            {
                grids->Add(grid.center, grid.size, grid.angle, grid.divisions, grid.drawBorder);
            }
        }
    }

    // Locked Grids
    {
        std::vector<UIGrid> xfer;
        if(ps.IsSaving())
        {
            for(int i = 0; i < lockedGrids->GetCount(); ++i)
            {
                xfer.push_back(lockedGrids->GetGrid(i));
            }
        }
        ps.Xfer("lockedGrids", xfer);
        if(ps.IsLoading())
        {
            lockedGrids->Clear();
            for(UIGrid& grid : xfer)
            {
                lockedGrids->Add(grid.center, grid.size, grid.angle, grid.divisions, grid.drawBorder);
            }
        }
    }

    // Hexagrams weren't added until version 2 of the save file.
    if(ps.GetFormatVersionNumber() >= 2)
    {
        // Hexagrams
        {
            std::vector<UIHexagram> xfer;
            if(ps.IsSaving())
            {
                for(int i = 0; i < hexagrams->GetCount(); ++i)
                {
                    xfer.push_back(hexagrams->GetHexagram(i));
                }
            }
            ps.Xfer("hexagrams", xfer);
            if(ps.IsLoading())
            {
                hexagrams->ClearHexagrams();
                for(UIHexagram& hexagram : xfer)
                {
                    hexagrams->AddHexagram(hexagram.center, hexagram.radius, hexagram.angle);
                }
            }
        }

        // Locked Hexagrams
        {
            std::vector<UIHexagram> xfer;
            if(ps.IsSaving())
            {
                for(int i = 0; i < lockedHexagrams->GetCount(); ++i)
                {
                    xfer.push_back(lockedHexagrams->GetHexagram(i));
                }
            }
            ps.Xfer("lockedHexagrams", xfer);
            if(ps.IsLoading())
            {
                lockedHexagrams->ClearHexagrams();
                for(UIHexagram& hexagram : xfer)
                {
                    lockedHexagrams->AddHexagram(hexagram.center, hexagram.radius, hexagram.angle);
                }
            }
        }
    }
}

Vector2 SidneyAnalyze::MapState::ToZoomedInPoint(const Vector2& pos)
{
    // Transform point from zoomed out map coordinate to zoomed in map coordinate.
    return (pos / kZoomedOutMapSize) * kZoomedInMapSize;
}

Vector2 SidneyAnalyze::MapState::ToZoomedOutPoint(const Vector2& pos)
{
    // Transform point from zoomed in map coordinate to zoomed out map coordinate.
    return (pos / kZoomedInMapSize) * kZoomedOutMapSize;
}

std::string SidneyAnalyze::MapState::GetPointLatLongText(const Vector2& zoomedInPos)
{
    // Get normalized position on map.
    Vector2 normPos = zoomedInPos / (kZoomedInMapSize - 1);

    // Convert to long/lat "points".
    // This is number of individual degrees between the min/max displayed on the map.
    const int kMaxLongitude = 521;
    const int kMaxLatitude = 281;
    const int kUnitsPerDegree = 60;
    int longValue = Math::RoundToInt(normPos.x * kMaxLongitude);
    int latValue = Math::RoundToInt(normPos.y * kMaxLatitude);

    // Convert to format of X'Y". Goes between 14'30" and 23'11".
    int longMajorValue = 0;
    int longMinorValue = 0;
    if(longValue < 30)
    {
        longMajorValue = 14;
        longMinorValue = longValue + 30;
    }
    else
    {
        longMajorValue = 15 + (longValue - 30) / kUnitsPerDegree;
        longMinorValue = (longValue - 30) % kUnitsPerDegree;
    }

    // Convert to format of X'Y". Goes between 52'33" and 57'14".
    int latMajorValue = 0;
    int latMinorValue = 0;
    if(latValue < 27)
    {
        latMajorValue = 52;
        latMinorValue = latValue + 33;
    }
    else
    {
        latMajorValue = 53 + (latValue - 27) / kUnitsPerDegree;
        latMinorValue = (latValue - 27) % kUnitsPerDegree;
    }

    // Format as a string and display as status text.
    std::string formatStr = SidneyUtil::GetAnalyzeLocalizer().GetText("MapLatLongText");
    return StringUtil::Format(formatStr.c_str(), 2, longMajorValue, longMinorValue, 42, latMajorValue, latMinorValue);
}

void SidneyAnalyze::MapState::AddShape(const std::string& shapeName)
{
    if(StringUtil::EqualsIgnoreCase(shapeName, "Triangle"))
    {
        // I don't think the game ever actually lets you place the triangle shape...
        // Grace says "I'm not ready for that shape yet."
        gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27NG1\", 1)");
    }
    else if(StringUtil::EqualsIgnoreCase(shapeName, "Circle"))
    {
        if(!gGameProgress.GetFlag("Aquarius"))
        {
            // If you try to place a Circle before Aquarius is done, Grace won't let you.
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27NG1\", 1)");
        }
        else if(gGameProgress.GetFlag("Pisces"))
        {
            // If you try to place after Pisces, Grace says another one isn't needed.
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27731\", 1)");
        }
        else
        {
            // During Pisces is the only time you can place a circle!
            const Vector2 kDefaultCirclePos(599.0f, 770.0f);
            const float kDefaultCircleRadius = 200.0f;
            zoomedIn.circles->AddCircle(kDefaultCirclePos, kDefaultCircleRadius);
            zoomedOut.circles->AddCircle(ToZoomedOutPoint(kDefaultCirclePos), (kDefaultCircleRadius / kZoomedInMapSize) * kZoomedOutMapSize);
        }
    }
    else if(StringUtil::EqualsIgnoreCase(shapeName, "Rectangle"))
    {
        if(!gGameProgress.GetFlag("Pisces"))
        {
            // If you haven't finished Pisces, Grace will say she's not ready for this shape.
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27NG1\", 1)");
        }
        else if(gGameProgress.GetFlag("Aries"))
        {
            // If you try to place after Aries, Grace says another one isn't needed.
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27731\", 1)");
        }
        else
        {
            // You can only place a Rectangle during Aries!
            const Vector2 kDefaultRectanglePos(599.0f, 770.0f);
            const Vector2 kDefaultRectangleSize(400.0f, 400.0f);
            zoomedIn.rectangles->AddRectangle(kDefaultRectanglePos, kDefaultRectangleSize, 0.0f);
            zoomedOut.rectangles->AddRectangle(ToZoomedOutPoint(kDefaultRectanglePos), ToZoomedOutPoint(kDefaultRectangleSize), 0.0f);
        }
    }
    else if(StringUtil::EqualsIgnoreCase(shapeName, "Hexagram"))
    {
        if(!gGameProgress.GetFlag("Leo") || !gGameProgress.GetFlag("Virgo"))
        {
            // If you haven't finished Leo and Virgo, Grace will say she's not ready for this shape.
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27NG1\", 1)");
        }
        else if(gGameProgress.GetFlag("Libra"))
        {
            // If you try to place after Libra, Grace says another one isn't needed.
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27731\", 1)");
        }
        else
        {
            // You can only place a hexagram during Libra!
            const Vector2 kDefaultHexagramPos(599.0f, 770.0f);
            const float kDefaultHexagramRadius = 400.0f;
            zoomedIn.hexagrams->AddHexagram(kDefaultHexagramPos, kDefaultHexagramRadius, 0.0f);
            zoomedOut.hexagrams->AddHexagram(ToZoomedOutPoint(kDefaultHexagramPos), (kDefaultHexagramRadius / kZoomedInMapSize) * kZoomedOutMapSize, 0.0f);
        }
    }
}

void SidneyAnalyze::MapState::EraseSelectedShape()
{
    if(selectedCircleIndex >= 0)
    {
        selectedCircleIndex = -1;
        zoomedIn.circles->ClearCircles();
        zoomedOut.circles->ClearCircles();
    }
    if(selectedRectangleIndex >= 0)
    {
        selectedRectangleIndex = -1;
        zoomedIn.rectangles->ClearRectangles();
        zoomedOut.rectangles->ClearRectangles();
    }
    if(selectedHexagramIndex >= 0)
    {
        selectedHexagramIndex = -1;
        zoomedIn.hexagrams->ClearHexagrams();
        zoomedOut.hexagrams->ClearHexagrams();
    }
}

bool SidneyAnalyze::MapState::IsAnyShapeSelected()
{
    return selectedCircleIndex >= 0 || selectedRectangleIndex >= 0 || selectedHexagramIndex >= 0;
}

void SidneyAnalyze::MapState::ClearShapeSelection()
{
    if(selectedCircleIndex >= 0)
    {
        selectedCircleIndex = -1;
        zoomedOut.circles->SetColor(kZoomedOutShapeColor);
    }
    if(selectedRectangleIndex >= 0)
    {
        selectedRectangleIndex = -1;
        zoomedOut.rectangles->SetColor(kZoomedOutShapeColor);
    }
    if(selectedHexagramIndex >= 0)
    {
        selectedHexagramIndex = -1;
        zoomedOut.hexagrams->SetColor(kZoomedOutShapeColor);
    }
}

void SidneyAnalyze::MapState::DrawGrid(uint8_t size, bool fillShape)
{
    if(fillShape)
    {
        // The game acts like you can do this whenever you want, but it's actually only possible to fill a Rectangle in one specific scenario.
        if(zoomedOut.lockedRectangles->GetCount() > 0)
        {
            const UIRectangle& rect = zoomedOut.lockedRectangles->GetRectangle(0);
            zoomedOut.grids->Add(rect.center, rect.size, rect.angle, size, false);
            zoomedIn.grids->Add(ToZoomedInPoint(rect.center), ToZoomedInPoint(rect.size), rect.angle, size, false);
        }
    }
    else
    {
        // Not filling a shape, so just draw one grid that fills the entire map.
        Vector2 gridRectSize = Vector2::One * kZoomedInMapSize;
        Vector2 gridRectCenter = gridRectSize * 0.5f;
        zoomedIn.grids->Add(gridRectCenter, gridRectSize, 0.0f, size, true);

        gridRectSize = Vector2::One * kZoomedOutMapSize;
        gridRectCenter = gridRectSize * 0.5f;
        zoomedOut.grids->Add(gridRectCenter, gridRectSize, 0.0f, size, true);
    }
}

void SidneyAnalyze::MapState::LockGrid()
{
    if(zoomedOut.grids->GetCount() > 0)
    {
        const UIGrid& grid = zoomedOut.grids->GetGrid(0);

        // Recreate grid in locked grids.
        zoomedOut.lockedGrids->Add(grid.center, grid.size, grid.angle, grid.divisions, grid.drawBorder);
        zoomedIn.lockedGrids->Add(ToZoomedInPoint(grid.center), ToZoomedInPoint(grid.size),
                                  grid.angle, grid.divisions, grid.drawBorder);

        // Clear non-locked grids.
        zoomedOut.grids->Clear();
        zoomedIn.grids->Clear();
    }
}

void SidneyAnalyze::MapState::ClearGrid()
{
    // Clear any placed grids.
    zoomedIn.grids->Clear();
    zoomedOut.grids->Clear();
}

void SidneyAnalyze::MapState::RefreshImages()
{
    bool siteVisible = gGameProgress.GetFlag("MarkedTheSite");
    zoomedIn.siteText[0]->SetEnabled(siteVisible);
    zoomedIn.siteText[1]->SetEnabled(siteVisible);
    zoomedOut.siteText[0]->SetEnabled(siteVisible);
    zoomedOut.siteText[1]->SetEnabled(siteVisible);

    bool serpentVisible = gGameProgress.GetFlag("PlacedSerpent");
    zoomedIn.serpentImage->SetEnabled(serpentVisible);
    zoomedOut.serpentImage->SetEnabled(serpentVisible);
}

void SidneyAnalyze::MapState::OnPersist(PersistState& ps)
{
    ps.Xfer(PERSIST_VAR(zoomedOut));
    ps.Xfer(PERSIST_VAR(zoomedIn));
}

void SidneyAnalyze::AnalyzeMap_Init()
{
    // Create a parent "map window" object that contains all the map analysis stuff.
    mAnalyzeMapWindow = new Actor("Analyze Map", TransformType::RectTransform);
    mAnalyzeMapWindow->GetTransform()->SetParent(mRoot->GetTransform());
    static_cast<RectTransform*>(mAnalyzeMapWindow->GetTransform())->SetAnchor(AnchorPreset::CenterStretch);
    static_cast<RectTransform*>(mAnalyzeMapWindow->GetTransform())->SetAnchoredPosition(0.0f, 0.0f);
    static_cast<RectTransform*>(mAnalyzeMapWindow->GetTransform())->SetSizeDelta(0.0f, 0.0f);

    // Create zoomed in map on left.
    {
        // Create an actor that represents the zoomed in window area.
        Actor* zoomedInMapWindow = new Actor("Zoomed In Map", TransformType::RectTransform);
        zoomedInMapWindow->GetTransform()->SetParent(mAnalyzeMapWindow->GetTransform());
        mMap.zoomedIn.button = zoomedInMapWindow->AddComponent<UIButton>();

        // Put a canvas on it with masking! This allows us to move around the child map background and have it be masked outside the window area.
        UICanvas* zoomedInMapCanvas = zoomedInMapWindow->AddComponent<UICanvas>(1);
        zoomedInMapCanvas->SetMasked(true);
        zoomedInMapCanvas->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
        zoomedInMapCanvas->GetRectTransform()->SetAnchoredPosition(9.0f, 73.0f);
        zoomedInMapCanvas->GetRectTransform()->SetSizeDelta(kZoomedInMapViewWidth, kZoomedInMapViewHeight);

        // Create big background image at full size.
        // Note that only a portion is visible due to the parent's canvas masking.
        mMap.zoomedIn.mapImage = UI::CreateWidgetActor<UIImage>("MapImage", zoomedInMapWindow);
        mMap.zoomedIn.mapImage->SetTexture(gAssetManager.LoadTexture("SIDNEYBIGMAP.BMP"), true);
        mMap.zoomedIn.mapImage->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);

        // Add "the site" text.
        // Do this before shapes/grids, since it should draw under those things.
        {
            mMap.zoomedIn.siteText[0] = UI::CreateWidgetActor<UIImage>("SiteText1", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.siteText[0]->SetTexture(gAssetManager.LoadTexture("MAPLG_THE.BMP"), true);
            mMap.zoomedIn.siteText[0]->SetColor(Color32(255, 255, 255, 128));
            mMap.zoomedIn.siteText[0]->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.siteText[0]->GetRectTransform()->SetAnchoredPosition(823.0f, 1039.0f);
            mMap.zoomedIn.siteText[0]->SetEnabled(false);

            mMap.zoomedIn.siteText[1] = UI::CreateWidgetActor<UIImage>("SiteText2", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.siteText[1]->SetTexture(gAssetManager.LoadTexture("MAPLG_SITE.BMP"), true);
            mMap.zoomedIn.siteText[1]->SetColor(Color32(255, 255, 255, 128));
            mMap.zoomedIn.siteText[1]->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.siteText[1]->GetRectTransform()->SetAnchoredPosition(792.0f, 983.0f);
            mMap.zoomedIn.siteText[1]->SetEnabled(false);
        }

        // Add serpent images.
        // Again, before shapes/grids so they appear above it.
        {
            mMap.zoomedIn.serpentImage = UI::CreateWidgetActor<UIImage>("Serpent", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.serpentImage->SetTexture(gAssetManager.LoadTexture("SERPENT.BMP"), true);
            mMap.zoomedIn.serpentImage->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.serpentImage->GetRectTransform()->SetAnchoredPosition(724.0f, 1159.0f);
            mMap.zoomedIn.serpentImage->SetEnabled(false);
        }

        // Create locked hexagrams renderer.
        {
            mMap.zoomedIn.lockedHexagrams = UI::CreateWidgetActor<UIHexagrams>("LockedHexagrams", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.lockedHexagrams->SetColor(kZoomedInLockedShapeColor);
            mMap.zoomedIn.lockedHexagrams->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.lockedHexagrams->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create hexagrams renderer.
        {
            mMap.zoomedIn.hexagrams = UI::CreateWidgetActor<UIHexagrams>("Hexagrams", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.hexagrams->SetColor(kZoomedInShapeColor);
            mMap.zoomedIn.hexagrams->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.hexagrams->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked grids renderer.
        {
            mMap.zoomedIn.lockedGrids = UI::CreateWidgetActor<UIGrids>("LockedGrids", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.lockedGrids->SetColor(kZoomedInLockedGridColor);
            mMap.zoomedIn.lockedGrids->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.lockedGrids->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create grids renderer.
        {
            mMap.zoomedIn.grids = UI::CreateWidgetActor<UIGrids>("Grids", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.grids->SetColor(kZoomedInShapeColor);
            mMap.zoomedIn.grids->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.grids->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked rectangles renderer.
        {
            mMap.zoomedIn.lockedRectangles = UI::CreateWidgetActor<UIRectangles>("LockedRectangles", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.lockedRectangles->SetColor(kZoomedInLockedShapeColor);
            mMap.zoomedIn.lockedRectangles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.lockedRectangles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create rectangles renderer.
        {
            mMap.zoomedIn.rectangles = UI::CreateWidgetActor<UIRectangles>("Rectangles", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.rectangles->SetColor(kZoomedInShapeColor);
            mMap.zoomedIn.rectangles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.rectangles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked circles renderer.
        {
            mMap.zoomedIn.lockedCircles = UI::CreateWidgetActor<UICircles>("LockedCircles", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.lockedCircles->SetColor(kZoomedInLockedShapeColor);
            mMap.zoomedIn.lockedCircles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.lockedCircles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create circles renderer.
        {
            mMap.zoomedIn.circles = UI::CreateWidgetActor<UICircles>("Circles", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.circles->SetColor(kZoomedInShapeColor);
            mMap.zoomedIn.circles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.circles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create lines renderer.
        {
            mMap.zoomedIn.lines = UI::CreateWidgetActor<UILines>("Lines", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.lines->SetColor(kZoomedInLineColor);
            mMap.zoomedIn.lines->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.lines->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked points renderer.
        {
            mMap.zoomedIn.lockedPoints = UI::CreateWidgetActor<UIPoints>("LockedPoints", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.lockedPoints->SetColor(kZoomedInLockedShapeColor);
            mMap.zoomedIn.lockedPoints->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.lockedPoints->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create points renderer.
        {
            mMap.zoomedIn.points = UI::CreateWidgetActor<UIPoints>("Points", mMap.zoomedIn.mapImage);
            mMap.zoomedIn.points->SetColor(kZoomedInShapeColor);
            mMap.zoomedIn.points->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedIn.points->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }
    }

    // Create zoomed out map on right.
    {
        // Create the map image itself.
        Actor* zoomedOutMapActor = new Actor("Zoomed Out Map", TransformType::RectTransform);
        zoomedOutMapActor->GetTransform()->SetParent(mAnalyzeMapWindow->GetTransform());

        mMap.zoomedOut.mapImage = zoomedOutMapActor->AddComponent<UIImage>();
        mMap.zoomedOut.mapImage->SetTexture(gAssetManager.LoadTexture("SIDNEYLITTLEMAP.BMP"));

        mMap.zoomedOut.mapImage->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
        mMap.zoomedOut.mapImage->GetRectTransform()->SetAnchoredPosition(289.0f, 55.0f);
        mMap.zoomedOut.mapImage->GetRectTransform()->SetSizeDelta(kZoomedOutMapSize, kZoomedOutMapSize);

        mMap.zoomedOut.button = zoomedOutMapActor->AddComponent<UIButton>();

        // The zoomed out map also needs a masking canvas so that moved shapes don't show outside the map border.
        UICanvas* zoomedOutMapCanvas = zoomedOutMapActor->AddComponent<UICanvas>(1);
        zoomedOutMapCanvas->SetMasked(true);

        // Add "the site" text.
        // Do this before shapes/grids, since it should draw under those things.
        {
            mMap.zoomedOut.siteText[0] = UI::CreateWidgetActor<UIImage>("SiteText1", zoomedOutMapActor);
            mMap.zoomedOut.siteText[0]->SetTexture(gAssetManager.LoadTexture("MAPSM_THE.BMP"), true);
            mMap.zoomedOut.siteText[0]->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.siteText[0]->GetRectTransform()->SetAnchoredPosition(204.0f, 254.0f);
            mMap.zoomedOut.siteText[0]->SetEnabled(false);

            mMap.zoomedOut.siteText[1] = UI::CreateWidgetActor<UIImage>("SiteText2", zoomedOutMapActor);
            mMap.zoomedOut.siteText[1]->SetTexture(gAssetManager.LoadTexture("MAPSM_SITE.BMP"), true);
            mMap.zoomedOut.siteText[1]->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.siteText[1]->GetRectTransform()->SetAnchoredPosition(196.0f, 242.0f);
            mMap.zoomedOut.siteText[1]->SetEnabled(false);
        }

        // Add serpent images.
        // Again, before shapes/grids so they appear above it.
        {
            mMap.zoomedOut.serpentImage = UI::CreateWidgetActor<UIImage>("Serpent", zoomedOutMapActor);
            mMap.zoomedOut.serpentImage->SetTexture(gAssetManager.LoadTexture("SERPLITMAP.BMP"), true);
            mMap.zoomedOut.serpentImage->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.serpentImage->GetRectTransform()->SetAnchoredPosition(178.0f, 290.0f);
            mMap.zoomedOut.serpentImage->SetEnabled(false);
        }

        // Create locked hexagrams renderer.
        {
            mMap.zoomedOut.lockedHexagrams = UI::CreateWidgetActor<UIHexagrams>("LockedHexagrams", zoomedOutMapActor);
            mMap.zoomedOut.lockedHexagrams->SetColor(kZoomedOutLockedShapeColor);
            mMap.zoomedOut.lockedHexagrams->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.lockedHexagrams->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create hexagrams renderer.
        {
            mMap.zoomedOut.hexagrams = UI::CreateWidgetActor<UIHexagrams>("Hexagrams", zoomedOutMapActor);
            mMap.zoomedOut.hexagrams->SetColor(kZoomedOutShapeColor);
            mMap.zoomedOut.hexagrams->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.hexagrams->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked grids renderer.
        {
            mMap.zoomedOut.lockedGrids = UI::CreateWidgetActor<UIGrids>("LockedGrids", zoomedOutMapActor);
            mMap.zoomedOut.lockedGrids->SetColor(kZoomedOutLockedGridColor);
            mMap.zoomedOut.lockedGrids->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.lockedGrids->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create grids renderer.
        {
            mMap.zoomedOut.grids = UI::CreateWidgetActor<UIGrids>("Grids", zoomedOutMapActor);
            mMap.zoomedOut.grids->SetColor(kZoomedOutShapeColor);
            mMap.zoomedOut.grids->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.grids->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked rectangles renderer.
        {
            mMap.zoomedOut.lockedRectangles = UI::CreateWidgetActor<UIRectangles>("LockedRectangles", zoomedOutMapActor);
            mMap.zoomedOut.lockedRectangles->SetColor(kZoomedOutLockedShapeColor);
            mMap.zoomedOut.lockedRectangles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.lockedRectangles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create rectangles renderer.
        {
            mMap.zoomedOut.rectangles = UI::CreateWidgetActor<UIRectangles>("Rectangles", zoomedOutMapActor);
            mMap.zoomedOut.rectangles->SetColor(kZoomedOutShapeColor);
            mMap.zoomedOut.rectangles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.rectangles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked circles renderer.
        {
            mMap.zoomedOut.lockedCircles = UI::CreateWidgetActor<UICircles>("LockedCircles", zoomedOutMapActor);
            mMap.zoomedOut.lockedCircles->SetColor(kZoomedOutLockedShapeColor);
            mMap.zoomedOut.lockedCircles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.lockedCircles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create circles renderer.
        {
            mMap.zoomedOut.circles = UI::CreateWidgetActor<UICircles>("Circles", zoomedOutMapActor);
            mMap.zoomedOut.circles->SetColor(kZoomedOutShapeColor);
            mMap.zoomedOut.circles->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.circles->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create lines renderer.
        {
            mMap.zoomedOut.lines = UI::CreateWidgetActor<UILines>("Lines", zoomedOutMapActor);
            mMap.zoomedOut.lines->SetColor(kZoomedOutLineColor);
            mMap.zoomedOut.lines->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.lines->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create locked points renderer.
        {
            mMap.zoomedOut.lockedPoints = UI::CreateWidgetActor<UIPoints>("LockedPoints", zoomedOutMapActor);
            mMap.zoomedOut.lockedPoints->SetColor(kZoomedOutLockedShapeColor);
            mMap.zoomedOut.lockedPoints->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.lockedPoints->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }

        // Create active points renderer.
        {
            mMap.zoomedOut.points = UI::CreateWidgetActor<UIPoints>("Points", zoomedOutMapActor);
            mMap.zoomedOut.points->SetColor(kZoomedOutShapeColor);
            mMap.zoomedOut.points->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
            mMap.zoomedOut.points->GetRectTransform()->SetAnchoredPosition(Vector2::Zero);
        }
    }

    // Create status text label.
    {
        mMapStatusLabel = UI::CreateWidgetActor<UILabel>("MapStatus", mAnalyzeMapWindow);
        mMapStatusLabel->SetFont(gAssetManager.LoadFont("SID_TEXT_14_GRN.FON"));
        mMapStatusLabel->GetRectTransform()->SetAnchor(AnchorPreset::BottomLeft);
        mMapStatusLabel->GetRectTransform()->SetAnchoredPosition(4.0f, 13.0f);
        mMapStatusLabel->SetEnabled(false);
    }

    // Hide by default.
    mAnalyzeMapWindow->SetActive(false);
}

void SidneyAnalyze::AnalyzeMap_EnterState()
{
    // Show the map view.
    mAnalyzeMapWindow->SetActive(true);

    // "Graphic" and "Map" dropdowns are available when analyzing a map. Text is not.
    mMenuBar.SetDropdownEnabled(kTextDropdownIdx, false);
    mMenuBar.SetDropdownEnabled(kGraphicDropdownIdx, true);
    mMenuBar.SetDropdownEnabled(kMapDropdownIdx, true);

    // "Graphic" choices are mostly grayed out.
    mMenuBar.SetDropdownChoiceEnabled(kGraphicDropdownIdx, kGraphicDropdown_ViewGeometryIdx, false);
    mMenuBar.SetDropdownChoiceEnabled(kGraphicDropdownIdx, kGraphicDropdown_RotateShapeIdx, false);
    mMenuBar.SetDropdownChoiceEnabled(kGraphicDropdownIdx, kGraphicDropdown_ZoomClarifyIdx, false);

    // Use & Erase Shape are enabled if we have any Shapes saved.
    mMenuBar.SetDropdownChoiceEnabled(kGraphicDropdownIdx, kGraphicDropdown_UseShapeIdx, mSidneyFiles->HasFileOfType(SidneyFileType::Shape));
    mMenuBar.SetDropdownChoiceEnabled(kGraphicDropdownIdx, kGraphicDropdown_EraseShapeIdx, mSidneyFiles->HasFileOfType(SidneyFileType::Shape));

    // Refresh site label visibility on map.
    mMap.RefreshImages();
}

void SidneyAnalyze::AnalyzeMap_Update(float deltaTime)
{
    if(!mAnalyzeMapWindow->IsActive()) { return; }

    // Hide map status label if enough time has passed.
    if(mMapStatusLabelTimer > 0.0f)
    {
        mMapStatusLabelTimer -= deltaTime;
        if(mMapStatusLabelTimer <= 0.0f)
        {
            mMapStatusLabel->SetEnabled(false);
        }
    }

    // Do not update maps or LSR progress if an action is active.
    if(gActionManager.IsActionPlaying())
    {
        mMenuBar.SetInteractive(false);
        return;
    }

    // Do not update maps or LSR progress when the analyze message is visible.
    if(mAnalyzePopup->IsActive())
    {
        mMenuBar.SetInteractive(false);
        return;
    }
    mMenuBar.SetInteractive(true);

    // Update interaction with the zoomed out map.
    AnalyzeMap_UpdateZoomedOutMap(deltaTime);

    // Update interaction with the zoomed in map.
    AnalyzeMap_UpdateZoomedInMap(deltaTime);

    // To complete Pisces, you must place three points on the map AND align a circle to them.
    bool aquariusDone = gGameProgress.GetFlag("Aquarius");
    bool piscesDone = gGameProgress.GetFlag("Pisces");
    bool ariesDone = gGameProgress.GetFlag("Aries");
    bool taurusDone = gGameProgress.GetFlag("Taurus");
    bool leoDone = gGameProgress.GetFlag("Leo");
    bool virgoDone = gGameProgress.GetFlag("Virgo");
    bool libraDone = gGameProgress.GetFlag("Libra");
    if(aquariusDone && !piscesDone)
    {
        Vector2 coustaussaPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPiscesCoustaussaPoint);
        Vector2 bezuPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPiscesBezuPoint);
        Vector2 bugarachPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPiscesBugarachPoint);
        if(coustaussaPoint != Vector2::Zero && bezuPoint != Vector2::Zero && bugarachPoint != Vector2::Zero)
        {
            const Vector2 kCircleCenter(168.875f, 174.5f);
            const float kCircleRadius = 121.0f;
            for(size_t i = 0; i < mMap.zoomedOut.circles->GetCirclesCount(); ++i)
            {
                const Circle& circle = mMap.zoomedOut.circles->GetCircle(i);

                float centerDiffSq = (circle.center - kCircleCenter).GetLengthSq();
                float radiusDiff = Math::Abs(circle.radius - kCircleRadius);
                if(centerDiffSq < 20 * 20 && radiusDiff < 4)
                {
                    // Put locked points on the zoomed in map.
                    mMap.zoomedIn.points->RemovePoint(coustaussaPoint);
                    mMap.zoomedIn.points->RemovePoint(bezuPoint);
                    mMap.zoomedIn.points->RemovePoint(bugarachPoint);

                    mMap.zoomedIn.lockedPoints->AddPoint(kPiscesCoustaussaPoint);
                    mMap.zoomedIn.lockedPoints->AddPoint(kPiscesBezuPoint);
                    mMap.zoomedIn.lockedPoints->AddPoint(kPiscesBugarachPoint);

                    // Same for the zoomed out map.
                    mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(coustaussaPoint));
                    mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(bezuPoint));
                    mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(bugarachPoint));

                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPiscesCoustaussaPoint));
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPiscesBezuPoint));
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPiscesBugarachPoint));

                    // Put locked circle on zoomed out map.
                    mMap.zoomedOut.circles->ClearCircles();
                    mMap.zoomedIn.circles->ClearCircles();
                    mMap.selectedCircleIndex = -1;

                    mMap.zoomedOut.lockedCircles->AddCircle(kCircleCenter, kCircleRadius);
                    mMap.zoomedIn.lockedCircles->AddCircle(mMap.ToZoomedInPoint(kCircleCenter), (kCircleRadius / kZoomedOutMapSize) * kZoomedInMapSize);

                    // Grace is excited that we figured it out.
                    gActionManager.ExecuteSheepAction("wait StartDialogue(\"02OAG2ZJU2\", 2)", [](const Action* action) {
                        gAudioManager.PlaySFX(gAssetManager.LoadAudio("CLOCKTIMEBLOCK.WAV"));
                    });

                    // Show confirmation message.
                    std::string message = StringUtil::Format(SidneyUtil::GetAnalyzeLocalizer().GetText("MapCircleConfirmNote").c_str(),
                                                             mMap.GetPointLatLongText(mMap.ToZoomedInPoint(kCircleCenter)).c_str());
                    ShowAnalyzeMessage(message);

                    // This also gets us the coordinates at the center of the circle as an inventory item.
                    gInventoryManager.AddInventoryItem("GRACE_COORDINATE_PAPER_1");

                    // We completed Pisces.
                    gGameProgress.SetFlag("Pisces");
                    gGameProgress.SetFlag("LockedCircle");
                    gGameProgress.ChangeScore("e_sidney_map_circle");
                    SidneyUtil::UpdateLSRState();
                }
            }
        }
    }
    else if(piscesDone && !ariesDone)
    {
        // To complete Aries, the player must place a Rectangle with a certain position and size.
        const Vector2 kRectangleCenter(168.875f, 174.5f);
        const float kRectangleSize = 242.0f;
        for(size_t i = 0; i < mMap.zoomedOut.rectangles->GetCount(); ++i)
        {
            const UIRectangle& rectangle = mMap.zoomedOut.rectangles->GetRectangle(i);

            float centerDiffSq = (rectangle.center - kRectangleCenter).GetLengthSq();
            float sizeDiff = Math::Abs(rectangle.size.x - kRectangleSize);
            if(centerDiffSq < 20 * 20 && sizeDiff < 4)
            {
                // Clear mouse click action, forcing player to stop manipulating the rectangle.
                // The rectangle remains selected however.
                mMap.zoomedOutClickAction = MapState::ClickAction::None;

                // Set the rectangle to the correct position/size.
                // Note that the rectangle IS NOT locked yet, since the player can still rotate it.
                UIRectangle correctRectangle;
                correctRectangle.center = kRectangleCenter;
                correctRectangle.size = Vector2::One * kRectangleSize;
                correctRectangle.angle = rectangle.angle;

                mMap.zoomedOut.rectangles->ClearRectangles();
                mMap.zoomedOut.rectangles->AddRectangle(correctRectangle.center, correctRectangle.size, correctRectangle.angle);

                mMap.zoomedIn.rectangles->ClearRectangles();
                mMap.zoomedIn.rectangles->AddRectangle(mMap.ToZoomedInPoint(correctRectangle.center),
                                                       mMap.ToZoomedInPoint(correctRectangle.size),
                                                       correctRectangle.angle);

                // Grace is excited that we figured it out.
                gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O7E2ZIS1\", 1)");

                // We completed Aries.
                gGameProgress.SetFlag("Aries");
                gGameProgress.SetFlag("SizedSquare");
                gGameProgress.ChangeScore("e_sidney_map_aries");
                SidneyUtil::UpdateLSRState();
            }
            //else
            //{
            //    printf("Center (%f, %f), Size (%f)\n", rectangle.center.x, rectangle.center.y, rectangle.size.x);
            //}
        }
    }
    else if(ariesDone && !taurusDone)
    {
        bool placedMeridianLine = mMap.zoomedIn.GetPlacedPointNearPoint(kTaurusSerresPoint, true) != Vector2::Zero;
        if(placedMeridianLine)
        {
            const UIRectangle& rectangle = mMap.zoomedOut.rectangles->GetRectangle(0);

            // Convert the rectangle's angle to the range 0 to 2Pi.
            float angle = rectangle.angle;
            while(angle < 0.0f)
            {
                angle += Math::k2Pi;
            }
            angle = Math::Mod(angle, Math::k2Pi);
            //printf("Rectangle angle is %f\n", angle);

            // There are four possible orientations that are considered correct.
            const float kRectangleRotation = 1.129951f;
            if(Math::Approximately(angle, kRectangleRotation, 0.1f) ||
               Math::Approximately(angle, kRectangleRotation + Math::kPiOver2, 0.1f) ||
               Math::Approximately(angle, kRectangleRotation + Math::kPi, 0.1f) ||
               Math::Approximately(angle, kRectangleRotation + Math::kPi + Math::kPiOver2, 0.1f))
            {
                // Clear click action, forcing the player to stop rotating the rectangle.
                mMap.zoomedOutClickAction = MapState::ClickAction::None;

                // Also clear the selection - rectangle can no longer be selected.
                mMap.selectedRectangleIndex = -1;

                // "Lock in" the correct rectangle.
                mMap.zoomedOut.rectangles->ClearRectangles();
                mMap.zoomedIn.rectangles->ClearRectangles();

                mMap.zoomedOut.lockedRectangles->AddRectangle(rectangle.center, rectangle.size, kRectangleRotation);
                mMap.zoomedIn.lockedRectangles->AddRectangle(mMap.ToZoomedInPoint(rectangle.center),
                                                             mMap.ToZoomedInPoint(rectangle.size),
                                                             kRectangleRotation);

                // Grace is excited that we figured it out. And time moves forward a bit!
                gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O7E2ZQB1\", 1)", [](const Action* action){
                    gAudioManager.PlaySFX(gAssetManager.LoadAudio("CLOCKTIMEBLOCK.WAV"));
                });

                // Taurus is done.
                gGameProgress.SetFlag("Taurus");
                gGameProgress.SetFlag("LockedSquare");
                gGameProgress.ChangeScore("e_sidney_map_taurus");
                SidneyUtil::UpdateLSRState();
            }
        }
    }
    else if(leoDone && virgoDone && !libraDone) // both Leo and Virgo must be done before you can complete Libra
    {
        // To complete Libra, the player must place a hexagram at a certain position, scale, and angle.
        const Vector2 kHexagramCenter(168.875f, 174.5f);
        const float kHexagramRadius = 121.0f;

        for(size_t i = 0; i < mMap.zoomedOut.hexagrams->GetCount(); ++i)
        {
            const UIHexagram& hexagram = mMap.zoomedOut.hexagrams->GetHexagram(i);

            // Center/radius checks are the same as for the circle earlier...
            float centerDiffSq = (hexagram.center - kHexagramCenter).GetLengthSq();
            float radiusDiff = Math::Abs(hexagram.radius - kHexagramRadius);
            if(centerDiffSq < 20 * 20 && radiusDiff < 4)
            {
                // The hexagram needs to be in a specific rotation. But there are several valid rotations to get the correct visual effect.
                // In practice, it comes down to being at 30 degrees, or any multiple of 30 degrees before/after that.

                // Convert angle to degrees, within 0-360.
                float angle = Math::Abs(Math::Mod(hexagram.angle, Math::k2Pi));
                float degrees = Math::ToDegrees(angle);

                // Check all valid rotations. If any match, you got it!
                const float kCloseEnoughDegrees = 2.0f;
                if(Math::Approximately(degrees, 30.0f, kCloseEnoughDegrees) ||
                   Math::Approximately(degrees, 90.0f, kCloseEnoughDegrees) ||
                   Math::Approximately(degrees, 150.0f, kCloseEnoughDegrees) ||
                   Math::Approximately(degrees, 210.0f, kCloseEnoughDegrees) ||
                   Math::Approximately(degrees, 270.0f, kCloseEnoughDegrees) ||
                   Math::Approximately(degrees, 330.0f, kCloseEnoughDegrees))
                {
                    // Clear click action, forcing the player to stop rotating the hexagram.
                    mMap.zoomedOutClickAction = MapState::ClickAction::None;

                    // Also clear the selection - hexagram can no longer be selected.
                    mMap.selectedHexagramIndex = -1;

                    // "Lock in" the correct hexagram.
                    mMap.zoomedOut.hexagrams->ClearHexagrams();
                    mMap.zoomedIn.hexagrams->ClearHexagrams();

                    mMap.zoomedOut.lockedHexagrams->AddHexagram(kHexagramCenter, kHexagramRadius, Math::ToRadians(30.0f));
                    mMap.zoomedIn.lockedHexagrams->AddHexagram(mMap.ToZoomedInPoint(kHexagramCenter),
                                                         (kHexagramRadius / kZoomedOutMapSize)* kZoomedInMapSize,
                                                         Math::ToRadians(30.0f));

                    // Grace says you got it right!
                    gActionManager.ExecuteDialogueAction("02O1K2ZC73", 2, [](const Action* action){

                        // This might end the timeblock.
                        SidneyUtil::CheckForceExitSidney307A();
                    });

                    // And you just finished Libra.
                    gGameProgress.ChangeScore("e_sidney_map_libra");
                    gGameProgress.SetFlag("Libra");
                    gGameProgress.SetFlag("LockedHexagram");
                    SidneyUtil::UpdateLSRState();
                }
            }
            //printf("Center (%f, %f), Size (%f), Angle (%f)\n", h"Lexagram.center.x, hexagram.center.y, hexagram.radius, angle);
        }
    }
}

void SidneyAnalyze::AnalyzeMap_UpdateZoomedOutMap(float deltaTime)
{
    // If mouse button is not pressed, clear click action.
    if(!gInputManager.IsMouseButtonPressed(InputManager::MouseButton::Left))
    {
        mMap.zoomedOutClickAction = MapState::ClickAction::None;
    }

    // If we're hovering the zoomed out map, we may be trying to interact with it.
    if(mMap.zoomedOut.button->IsHovered())
    {
        Vector2 zoomedOutMapPos = mMap.zoomedOut.GetLocalMousePos();
        //printf("%f, %f\n", zoomedOutMapPos.x, zoomedOutMapPos.y);

        // If hovering a point (locked or not) with no selected shape, update map status text with that point's lat/long.
        if(!mMap.IsAnyShapeSelected())
        {
            bool setText = false;
            for(size_t i = 0; i < mMap.zoomedOut.points->GetPointsCount(); ++i)
            {
                const Vector2& point = mMap.zoomedOut.points->GetPoint(i);
                float distToPointSq = (zoomedOutMapPos - point).GetLengthSq();
                if(distToPointSq < kHoverPointDistSq)
                {
                    AnalyzeMap_SetPointStatusText("MapHoverPointNote", mMap.ToZoomedInPoint(point));
                    setText = true;
                    break;
                }
            }
            if(!setText)
            {
                for(size_t i = 0; i < mMap.zoomedOut.lockedPoints->GetPointsCount(); ++i)
                {
                    const Vector2& point = mMap.zoomedOut.lockedPoints->GetPoint(i);
                    float distToPointSq = (zoomedOutMapPos - point).GetLengthSq();
                    if(distToPointSq < kHoverPointDistSq)
                    {
                        AnalyzeMap_SetPointStatusText("MapHoverPointNote", mMap.ToZoomedInPoint(point));
                        setText = true;
                        break;
                    }
                }
            }
        }

        // See if we clicked on a selectable shape. If so, it causes us to select it.
        if(gInputManager.IsMouseButtonLeadingEdge(InputManager::MouseButton::Left))
        {
            // Check for selecting a circle.
            for(size_t i = 0; i < mMap.zoomedOut.circles->GetCirclesCount(); ++i)
            {
                // The click pos must be along the edge of the circle, not in the center.
                const Circle& circle = mMap.zoomedOut.circles->GetCircle(i);
                Vector2 edgePoint = circle.GetClosestSurfacePoint(zoomedOutMapPos);
                if((edgePoint - zoomedOutMapPos).GetLengthSq() < 2.5f * 2.5f)
                {
                    if(mMap.selectedCircleIndex != i)
                    {
                        mMap.ClearShapeSelection();
                        mMap.selectedCircleIndex = i;
                        mMap.zoomedOut.circles->SetColor(kZoomedOutSelectedShapeColor);
                        mMap.zoomedOutClickAction = MapState::ClickAction::SelectShape;
                        break;
                    }
                }
            }

            // Check for selecting a rectangle.
            for(size_t i = 0; i < mMap.zoomedOut.rectangles->GetCount(); ++i)
            {
                const UIRectangle& rectangle = mMap.zoomedOut.rectangles->GetRectangle(i);
                if(IsPointOnEdgeOfRectangle(rectangle, zoomedOutMapPos))
                {
                    if(mMap.selectedRectangleIndex != i)
                    {
                        mMap.ClearShapeSelection();
                        mMap.selectedRectangleIndex = i;
                        mMap.zoomedOut.rectangles->SetColor(kZoomedOutSelectedShapeColor);
                        mMap.zoomedOutClickAction = MapState::ClickAction::SelectShape;
                        break;
                    }
                }
            }

            // Check for selecting a hexagram.
            for(size_t i = 0; i < mMap.zoomedOut.hexagrams->GetCount(); ++i)
            {
                const UIHexagram& hexagram = mMap.zoomedOut.hexagrams->GetHexagram(i);
                if(IsPointOnEdgeOfHexagram(hexagram, zoomedOutMapPos))
                {
                    if(mMap.selectedHexagramIndex != i)
                    {
                        mMap.ClearShapeSelection();
                        mMap.selectedHexagramIndex = i;
                        mMap.zoomedOut.hexagrams->SetColor(kZoomedOutSelectedShapeColor);
                        mMap.zoomedOutClickAction = MapState::ClickAction::SelectShape;
                        break;
                    }
                }
            }
        }

        // This isn't frequently used, but right-clicking actually de-selects the current shape.
        if(gInputManager.IsMouseButtonLeadingEdge(InputManager::MouseButton::Right))
        {
            mMap.ClearShapeSelection();
        }

        // We have a selected shape. See if we're hovering it and change the cursor if so.
        bool moveShapeCursor = false;
        bool resizeShapeCursor = false;
        bool rotateShapeCursor = false;
        if(mMap.IsAnyShapeSelected())
        {
            if(mMap.selectedCircleIndex >= 0)
            {
                const Circle& circle = mMap.zoomedOut.circles->GetCircle(mMap.selectedCircleIndex);
                Vector2 edgePoint = circle.GetClosestSurfacePoint(zoomedOutMapPos);
                if((edgePoint - zoomedOutMapPos).GetLengthSq() < 2.5f * 2.5f || mMap.zoomedOutClickAction == MapState::ClickAction::ResizeShape)
                {
                    resizeShapeCursor = true;

                }
                else if(circle.ContainsPoint(zoomedOutMapPos) || mMap.zoomedOutClickAction == MapState::ClickAction::MoveShape)
                {
                    moveShapeCursor = true;
                }
            }
            else if(mMap.selectedRectangleIndex >= 0)
            {
                const UIRectangle& rectangle = mMap.zoomedOut.rectangles->GetRectangle(mMap.selectedRectangleIndex);
                if(IsPointOnEdgeOfRectangle(rectangle, zoomedOutMapPos))
                {
                    resizeShapeCursor = true;

                    // You are only allowed to resize the rectangle before Aries is complete.
                    if(gGameProgress.GetFlag("Aries"))
                    {
                        resizeShapeCursor = false;
                    }
                }
                else if(IsPointInsideRectangle(rectangle, zoomedOutMapPos))
                {
                    moveShapeCursor = true;

                    // You are only allowed to move the rectangle before Aries is complete.
                    if(gGameProgress.GetFlag("Aries"))
                    {
                        moveShapeCursor = false;
                    }
                }
                else
                {
                    rotateShapeCursor = true;
                }
            }
            else if(mMap.selectedHexagramIndex >= 0)
            {
                const UIHexagram& hexagram = mMap.zoomedOut.hexagrams->GetHexagram(mMap.selectedHexagramIndex);
                if(IsPointOnEdgeOfHexagram(hexagram, zoomedOutMapPos))
                {
                    resizeShapeCursor = true;
                }
                else if((zoomedOutMapPos - hexagram.center).GetLengthSq() < hexagram.radius * hexagram.radius)
                {
                    moveShapeCursor = true;
                }
                else
                {
                    rotateShapeCursor = true;
                }
            }
        }

        if(resizeShapeCursor || mMap.zoomedOutClickAction == MapState::ClickAction::ResizeShape)
        {
            gCursorManager.UseCustomCursor(gAssetManager.LoadCursor("C_DRAGRESIZED1.CUR"), 1);
        }
        else if(moveShapeCursor || mMap.zoomedOutClickAction == MapState::ClickAction::MoveShape)
        {
            gCursorManager.UseCustomCursor(gAssetManager.LoadCursor("C_DRAGMOVE.CUR"), 1);
        }
        else if(rotateShapeCursor || mMap.zoomedOutClickAction == MapState::ClickAction::RotateShape)
        {
            gCursorManager.UseCustomCursor(gAssetManager.LoadCursor("C_TURNRIGHT.CUR"), 1);
        }
        else
        {
            gCursorManager.UseDefaultCursor();
        }

        // Upon clicking the left mouse button, we commit to a certain action until the mouse button is later released.
        if(gInputManager.IsMouseButtonLeadingEdge(InputManager::MouseButton::Left) && mMap.zoomedOutClickAction == MapState::ClickAction::None)
        {
            if(moveShapeCursor)
            {
                mMap.zoomedOutClickAction = MapState::ClickAction::MoveShape;
            }
            else if(resizeShapeCursor)
            {
                mMap.zoomedOutClickAction = MapState::ClickAction::ResizeShape;
            }
            else if(rotateShapeCursor)
            {
                mMap.zoomedOutClickAction = MapState::ClickAction::RotateShape;
            }
            else
            {
                mMap.zoomedOutClickAction = MapState::ClickAction::FocusMap;
            }

            mMap.zoomedOutClickActionPos = zoomedOutMapPos;
            if(mMap.selectedCircleIndex >= 0)
            {
                mMap.zoomedOutClickShapeCenter = mMap.zoomedOut.circles->GetCircle(mMap.selectedCircleIndex).center;
            }
            if(mMap.selectedRectangleIndex >= 0)
            {
                mMap.zoomedOutClickShapeCenter = mMap.zoomedOut.rectangles->GetRectangle(mMap.selectedRectangleIndex).center;
            }
            if(mMap.selectedHexagramIndex >= 0)
            {
                mMap.zoomedOutClickShapeCenter = mMap.zoomedOut.hexagrams->GetHexagram(mMap.selectedHexagramIndex).center;
            }
        }

        if(gInputManager.IsMouseButtonPressed(InputManager::MouseButton::Left))
        {
            if(mMap.zoomedOutClickAction == MapState::ClickAction::MoveShape)
            {
                if(mMap.selectedCircleIndex >= 0)
                {
                    Circle zoomedOutCircle = mMap.zoomedOut.circles->GetCircle(mMap.selectedCircleIndex);

                    Vector3 expectedOffset = mMap.zoomedOutClickShapeCenter - mMap.zoomedOutClickActionPos;
                    zoomedOutCircle.center = zoomedOutMapPos + expectedOffset;

                    mMap.zoomedOut.circles->ClearCircles();
                    mMap.zoomedOut.circles->AddCircle(zoomedOutCircle.center, zoomedOutCircle.radius);

                    mMap.zoomedIn.circles->ClearCircles();
                    mMap.zoomedIn.circles->AddCircle(mMap.ToZoomedInPoint(zoomedOutCircle.center),
                                                     (zoomedOutCircle.radius / kZoomedOutMapSize) * kZoomedInMapSize);
                }
                if(mMap.selectedRectangleIndex >= 0)
                {
                    UIRectangle zoomedOutRectangle = mMap.zoomedOut.rectangles->GetRectangle(mMap.selectedRectangleIndex);

                    Vector3 expectedOffset = mMap.zoomedOutClickShapeCenter - mMap.zoomedOutClickActionPos;
                    zoomedOutRectangle.center = zoomedOutMapPos + expectedOffset;

                    mMap.zoomedOut.rectangles->ClearRectangles();
                    mMap.zoomedOut.rectangles->AddRectangle(zoomedOutRectangle.center, zoomedOutRectangle.size, zoomedOutRectangle.angle);

                    mMap.zoomedIn.rectangles->ClearRectangles();
                    mMap.zoomedIn.rectangles->AddRectangle(mMap.ToZoomedInPoint(zoomedOutRectangle.center),
                                                           mMap.ToZoomedInPoint(zoomedOutRectangle.size),
                                                           zoomedOutRectangle.angle);
                }
                if(mMap.selectedHexagramIndex >= 0)
                {
                    UIHexagram zoomedOutHexagram = mMap.zoomedOut.hexagrams->GetHexagram(mMap.selectedHexagramIndex);

                    Vector3 expectedOffset = mMap.zoomedOutClickShapeCenter - mMap.zoomedOutClickActionPos;
                    zoomedOutHexagram.center = zoomedOutMapPos + expectedOffset;

                    mMap.zoomedOut.hexagrams->ClearHexagrams();
                    mMap.zoomedOut.hexagrams->AddHexagram(zoomedOutHexagram.center, zoomedOutHexagram.radius, zoomedOutHexagram.angle);

                    mMap.zoomedIn.hexagrams->ClearHexagrams();
                    mMap.zoomedIn.hexagrams->AddHexagram(mMap.ToZoomedInPoint(zoomedOutHexagram.center),
                                                         (zoomedOutHexagram.radius / kZoomedOutMapSize)* kZoomedInMapSize,
                                                         zoomedOutHexagram.angle);
                }
            }
            else if(mMap.zoomedOutClickAction == MapState::ClickAction::ResizeShape)
            {
                if(mMap.selectedCircleIndex >= 0)
                {
                    Circle zoomedOutCircle = mMap.zoomedOut.circles->GetCircle(mMap.selectedCircleIndex);
                    zoomedOutCircle.radius = (zoomedOutCircle.center - zoomedOutMapPos).GetLength();
                    mMap.zoomedOut.circles->ClearCircles();
                    mMap.zoomedOut.circles->AddCircle(zoomedOutCircle.center, zoomedOutCircle.radius);

                    mMap.zoomedIn.circles->ClearCircles();
                    mMap.zoomedIn.circles->AddCircle(mMap.ToZoomedInPoint(zoomedOutCircle.center),
                                                     (zoomedOutCircle.radius / kZoomedOutMapSize) * kZoomedInMapSize);
                }
                if(mMap.selectedRectangleIndex >= 0)
                {
                    UIRectangle zoomedOutRectangle = mMap.zoomedOut.rectangles->GetRectangle(mMap.selectedRectangleIndex);

                    Vector2 mouseMoveOffset = zoomedOutMapPos - mMap.zoomedOutClickActionPos;
                    Vector2 centerToPos = zoomedOutMapPos - zoomedOutRectangle.center;

                    float distChange = mouseMoveOffset.GetLength();
                    if(Vector2::Dot(mouseMoveOffset, centerToPos) < 0)
                    {
                        distChange *= -1.0f;
                    }
                    mMap.zoomedOutClickActionPos = zoomedOutMapPos;

                    zoomedOutRectangle.size.x += distChange;
                    zoomedOutRectangle.size.y += distChange;

                    mMap.zoomedOut.rectangles->ClearRectangles();
                    mMap.zoomedOut.rectangles->AddRectangle(zoomedOutRectangle.center, zoomedOutRectangle.size, zoomedOutRectangle.angle);

                    mMap.zoomedIn.rectangles->ClearRectangles();
                    mMap.zoomedIn.rectangles->AddRectangle(mMap.ToZoomedInPoint(zoomedOutRectangle.center),
                                                           mMap.ToZoomedInPoint(zoomedOutRectangle.size),
                                                           zoomedOutRectangle.angle);
                }
                if(mMap.selectedHexagramIndex >= 0)
                {
                    UIHexagram zoomedOutHexagram = mMap.zoomedOut.hexagrams->GetHexagram(mMap.selectedHexagramIndex);

                    Vector2 mouseMoveOffset = zoomedOutMapPos - mMap.zoomedOutClickActionPos;
                    Vector2 centerToPos = zoomedOutMapPos - zoomedOutHexagram.center;

                    float distChange = mouseMoveOffset.GetLength();
                    if(Vector2::Dot(mouseMoveOffset, centerToPos) < 0)
                    {
                        distChange *= -1.0f;
                    }
                    mMap.zoomedOutClickActionPos = zoomedOutMapPos;

                    zoomedOutHexagram.radius += distChange;

                    mMap.zoomedOut.hexagrams->ClearHexagrams();
                    mMap.zoomedOut.hexagrams->AddHexagram(zoomedOutHexagram.center, zoomedOutHexagram.radius, zoomedOutHexagram.angle);

                    mMap.zoomedIn.hexagrams->ClearHexagrams();
                    mMap.zoomedIn.hexagrams->AddHexagram(mMap.ToZoomedInPoint(zoomedOutHexagram.center),
                                                         (zoomedOutHexagram.radius / kZoomedOutMapSize)* kZoomedInMapSize,
                                                         zoomedOutHexagram.angle);
                }
            }
            else if(mMap.zoomedOutClickAction == MapState::ClickAction::RotateShape)
            {
                if(mMap.selectedRectangleIndex >= 0)
                {
                    UIRectangle zoomedOutRectangle = mMap.zoomedOut.rectangles->GetRectangle(mMap.selectedRectangleIndex);

                    Vector2 prevCenterToMouse = Vector2::Normalize(mMap.zoomedOutClickActionPos - mMap.zoomedOutClickShapeCenter);
                    Vector2 centerToMouse = Vector2::Normalize(zoomedOutMapPos - mMap.zoomedOutClickShapeCenter);
                    mMap.zoomedOutClickActionPos = zoomedOutMapPos;

                    float angle = Math::Acos(Vector2::Dot(prevCenterToMouse, centerToMouse));
                    Vector3 cross = Vector3::Cross(prevCenterToMouse, centerToMouse);
                    if(cross.z < 0)
                    {
                        angle *= -1.0f;
                    }
                    zoomedOutRectangle.angle += angle;

                    mMap.zoomedOut.rectangles->ClearRectangles();
                    mMap.zoomedOut.rectangles->AddRectangle(zoomedOutRectangle.center, zoomedOutRectangle.size, zoomedOutRectangle.angle);

                    mMap.zoomedIn.rectangles->ClearRectangles();
                    mMap.zoomedIn.rectangles->AddRectangle(mMap.ToZoomedInPoint(zoomedOutRectangle.center),
                                                           mMap.ToZoomedInPoint(zoomedOutRectangle.size),
                                                           zoomedOutRectangle.angle);
                }
                if(mMap.selectedHexagramIndex >= 0)
                {
                    UIHexagram zoomedOutHexagram = mMap.zoomedOut.hexagrams->GetHexagram(mMap.selectedHexagramIndex);

                    Vector2 prevCenterToMouse = Vector2::Normalize(mMap.zoomedOutClickActionPos - mMap.zoomedOutClickShapeCenter);
                    Vector2 centerToMouse = Vector2::Normalize(zoomedOutMapPos - mMap.zoomedOutClickShapeCenter);
                    mMap.zoomedOutClickActionPos = zoomedOutMapPos;

                    float angle = Math::Acos(Vector2::Dot(prevCenterToMouse, centerToMouse));
                    Vector3 cross = Vector3::Cross(prevCenterToMouse, centerToMouse);
                    if(cross.z < 0)
                    {
                        angle *= -1.0f;
                    }
                    zoomedOutHexagram.angle += angle;

                    mMap.zoomedOut.hexagrams->ClearHexagrams();
                    mMap.zoomedOut.hexagrams->AddHexagram(zoomedOutHexagram.center, zoomedOutHexagram.radius, zoomedOutHexagram.angle);

                    mMap.zoomedIn.hexagrams->ClearHexagrams();
                    mMap.zoomedIn.hexagrams->AddHexagram(mMap.ToZoomedInPoint(zoomedOutHexagram.center),
                                                         (zoomedOutHexagram.radius / kZoomedOutMapSize)* kZoomedInMapSize,
                                                         zoomedOutHexagram.angle);
                }
            }
            else if(mMap.zoomedOutClickAction == MapState::ClickAction::FocusMap)
            {
                Vector2 zoomedInMapPos = mMap.ToZoomedInPoint(zoomedOutMapPos);
                //printf("%f, %f\n", zoomedInPos.x, zoomedInPos.y);

                // The zoomed in view should center on the zoomed in pos.
                Vector2 halfViewSize(kZoomedInMapViewWidth * 0.5f, kZoomedInMapViewHeight * 0.5f);
                Rect zoomedInRect(zoomedInMapPos - halfViewSize, zoomedInMapPos + halfViewSize);

                // Make sure the zoomed in rect is within the bounds of the map.
                Vector2 zoomedInRectMin = zoomedInRect.GetMin();
                Vector2 zoomedInRectMax = zoomedInRect.GetMax();
                if(zoomedInRectMin.x < 0)
                {
                    zoomedInRect.x += Math::Abs(zoomedInRectMin.x);
                }
                if(zoomedInRectMin.y < 0)
                {
                    zoomedInRect.y += Math::Abs(zoomedInRectMin.y);
                }
                if(zoomedInRectMax.x >= kZoomedInMapSize)
                {
                    zoomedInRect.x -= (zoomedInRectMax.x - kZoomedInMapSize);
                }
                if(zoomedInRectMax.y >= kZoomedInMapSize)
                {
                    zoomedInRect.y -= (zoomedInRectMax.y - kZoomedInMapSize);
                }

                // Position the map based on the zoomed in rect.
                mMap.zoomedIn.mapImage->GetRectTransform()->SetAnchoredPosition(-zoomedInRect.GetMin());
            }
        }
    }
}

void SidneyAnalyze::AnalyzeMap_UpdateZoomedInMap(float deltaTime)
{
    // Clicking on the zoomed in map may perform an action in some cases.
    if(mMap.zoomedIn.button->IsHovered() && gInputManager.IsMouseButtonLeadingEdge(InputManager::MouseButton::Left))
    {
        if(mMap.enteringPoints)
        {
            // Add point to zoomed in map at click pos.
            Vector2 zoomedInMapPos = mMap.zoomedIn.GetLocalMousePos();
            mMap.zoomedIn.points->AddPoint(zoomedInMapPos);
            printf("Add pt at %f, %f\n", zoomedInMapPos.x, zoomedInMapPos.y);

            // Also convert to zoomed out map position and add point there.
            mMap.zoomedOut.points->AddPoint(mMap.ToZoomedOutPoint(zoomedInMapPos));

            // When you place a point, the latitude/longitude of the point are displayed on-screen.
            AnalyzeMap_SetPointStatusText("MapEnterPointNote", zoomedInMapPos);

            // There's one part (during Scorpio) when entering a point immediately illicits a response from Grace.
            if(gGameProgress.GetFlag("Libra") && !gGameProgress.GetFlag("Scorpio") && gGameProgress.GetFlag("PlacedTempleDivisions"))
            {
                const Vector2 kSitePoint(830.0f, 1027.0f);
                Vector2 sitePoint = mMap.zoomedIn.GetPlacedPointNearPoint(kSitePoint);
                if(sitePoint != Vector2::Zero)
                {
                    // Replace placed point with locked actual point.
                    mMap.zoomedIn.points->RemovePoint(zoomedInMapPos);
                    mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(zoomedInMapPos));
                    mMap.zoomedIn.lockedPoints->AddPoint(kSitePoint);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kSitePoint));

                    // Change score and apply flags.
                    gGameProgress.ChangeScore("e_sidney_map_scorpio");
                    gGameProgress.SetFlag("MarkedTheSite");
                    gGameProgress.SetFlag("Scorpio");
                    SidneyUtil::UpdateLSRState();

                    // Grace says "that's it, that's the site, I'll write down the coords."
                    gActionManager.ExecuteDialogueAction("02O8O2ZRA1", 2, [this](const Action* action){

                        // This popup is displayed a bit off-center.
                        RectTransform* popupRT = static_cast<RectTransform*>(mSetTextPopup->GetTransform());
                        popupRT->SetAnchor(AnchorPreset::TopLeft);
                        popupRT->SetAnchoredPosition(120.0f, -127.5f);

                        // Show the popup.
                        mSetTextPopup->Show(SidneyUtil::GetAnalyzeLocalizer().GetText("SiteTextTitle"),
                                            SidneyUtil::GetAnalyzeLocalizer().GetText("SiteTextPrompt"),
                                            SidneyUtil::GetAnalyzeLocalizer().GetText("SiteText"),
                        [this](){
                            // Refresh the map so "The Site" label appears.
                            mMap.RefreshImages();
                        });
                    });
                }

            }
        }
    }
}

void SidneyAnalyze::AnalyzeMap_OnAnalyzeButtonPressed()
{
    bool didValidAnalyzeAction = false;

    // The map analysis behavior depends on what part of the LSR riddle we're on.
    bool aquariusDone = gGameProgress.GetFlag("Aquarius");
    bool piscesDone = gGameProgress.GetFlag("Pisces");
    bool ariesDone = gGameProgress.GetFlag("Aries");
    bool taurusDone = gGameProgress.GetFlag("Taurus");
    bool geminiDone = gGameProgress.GetFlag("Gemini");
    bool cancerDone = gGameProgress.GetFlag("Cancer");
    bool leoDone = gGameProgress.GetFlag("Leo");
    bool virgoDone = gGameProgress.GetFlag("Virgo");
    bool libraDone = gGameProgress.GetFlag("Libra");
    bool scorpioDone = gGameProgress.GetFlag("Scorpio");
    bool ophiuchusDone = gGameProgress.GetFlag("Ophiuchus");
    bool sagitariusDone = gGameProgress.GetFlag("Sagittarius");
    if(!aquariusDone) // Working on Aquarius
    {
        // Player must place two points near enough to these points and press "Analyze" to pass Aquarius.
        const Vector2 kRLCPoint(266.0f, 952.0f);
        const Vector2 kCDBPoint(653.0f, 1060.0f);
        const Vector2 kSunLineEndPoint(1336.0f, 1249.0f);

        // See if two placed points meet the criteria to finish Aquarius.
        Vector2 rlcPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kRLCPoint);
        Vector2 cdbPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kCDBPoint);
        if(rlcPoint != Vector2::Zero && cdbPoint != Vector2::Zero)
        {
            // Grace says "Cool!"
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O3H2Z7F3\", 1)");

            // Show message confirming correct points.
            ShowAnalyzeMessage("MapLine1Note");

            // Correct points move from "active" points to "locked" points.
            mMap.zoomedIn.points->RemovePoint(rlcPoint);
            mMap.zoomedIn.lockedPoints->AddPoint(kRLCPoint);

            mMap.zoomedIn.points->RemovePoint(cdbPoint);
            mMap.zoomedIn.lockedPoints->AddPoint(kCDBPoint);

            mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(rlcPoint));
            mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kRLCPoint));

            mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(cdbPoint));
            mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kCDBPoint));

            // Add sun line through the placed points.
            mMap.zoomedIn.lines->AddLine(kRLCPoint, kSunLineEndPoint);
            mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kRLCPoint),
                                          mMap.ToZoomedOutPoint(kSunLineEndPoint));

            // Done with Aquarius!
            gGameProgress.SetFlag("Aquarius");
            gGameProgress.SetFlag("PlacedSunriseLine");
            gGameProgress.ChangeScore("e_sidney_map_aquarius");
            SidneyUtil::UpdateLSRState();
        }
    }
    else if(aquariusDone && !piscesDone) // Working on Pisces
    {
        // Find placed points that are close enough to the desired points.
        Vector2 coustaussaPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPiscesCoustaussaPoint);
        Vector2 bezuPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPiscesBezuPoint);
        Vector2 bugarachPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPiscesBugarachPoint);

        // If you place all the points, you get a different analysis message.
        // But you still need to place the Circle shape to solve Pisces!
        if(coustaussaPoint != Vector2::Zero && bezuPoint != Vector2::Zero && bugarachPoint != Vector2::Zero)
        {
            // Says "several possible linkages - use shapes or more points."
            ShowAnalyzeMessage("MapSeveralPossNote");
        }
    }
    else if((piscesDone && !ariesDone) || (ariesDone && !taurusDone)) // Working on Aries OR Taurus
    {
        // The game allows you to put down this line early in Aries, but it must be done to complete Taurus.
        // Find placed points that are close enough to the desired points.
        Vector2 serresPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kTaurusSerresPoint);
        Vector2 meridianPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kTaurusMeridianPoint);
        if(serresPoint != Vector2::Zero && meridianPoint != Vector2::Zero)
        {
            // Says "line is tangential to the circle."
            ShowAnalyzeMessage("MapLine2Note");

            // Remove points placed by the player.
            mMap.zoomedIn.points->RemovePoint(serresPoint);
            mMap.zoomedIn.points->RemovePoint(meridianPoint);
            mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(serresPoint));
            mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(meridianPoint));

            // It's possible for the player to place these points multiple times.
            // But only the first placement elicits exclamations from Grace.
            bool alreadyPlacedPoints = mMap.zoomedIn.GetPlacedPointNearPoint(kTaurusSerresPoint, true) != Vector2::Zero;
            if(!alreadyPlacedPoints)
            {
                // Grace says "Oh yeah, that's what the riddle means."
                gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O3H2ZQB2\", 1)");

                // Add locked points.
                mMap.zoomedIn.lockedPoints->AddPoint(kTaurusSerresPoint);
                mMap.zoomedIn.lockedPoints->AddPoint(kTaurusMeridianPoint);
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kTaurusSerresPoint));
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kTaurusMeridianPoint));

                // Place a line segment between the points.
                mMap.zoomedIn.lines->AddLine(kTaurusSerresPoint, kTaurusMeridianPoint);
                mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kTaurusSerresPoint),
                                              mMap.ToZoomedOutPoint(kTaurusMeridianPoint));

                // Update game state.
                gGameProgress.SetFlag("PlacedMeridianLine");
                gGameProgress.ChangeScore("e_sidney_map_serres");

                // NOTE: doing this doesn't complete Taurus - the player must rotate the square to align with these points.
            }
        }
    }
    else if(geminiDone && cancerDone && (!leoDone || !virgoDone)) // working on Leo or Virgo
    {
        // Players are logically supposed to complete Leo before Virgo, but they can be done in either order.

        // There's also the scenario to cover where the player placed the points for BOTH of these at the same time (tricky tricky).
        // In that case, Leo should complete first, and then Virgo on another button press.

        // First, checkif leo was completed, if not already done.
        bool justCompletedLeo = false;
        if(!leoDone)
        {
            // Player must place two points near enough to these points and press "Analyze" to pass Leo.
            const Vector2 kLermitagePoint(676.0f, 698.0f);
            const Vector2 kPoussinTombPoint(938.0f, 1207.0f);

            // See if two placed points meet the criteria to finish Leo.
            Vector2 lermitagePoint = mMap.zoomedIn.GetPlacedPointNearPoint(kLermitagePoint);
            Vector2 poussinTombPoint = mMap.zoomedIn.GetPlacedPointNearPoint(kPoussinTombPoint);
            if(lermitagePoint != Vector2::Zero && poussinTombPoint != Vector2::Zero)
            {
                // Says "line passes through meridian at sunrise line."
                ShowAnalyzeMessage("MapLine3Note", Vector2(), HorizontalAlignment::Center);

                // Remove points placed by the player.
                mMap.zoomedIn.points->RemovePoint(lermitagePoint);
                mMap.zoomedIn.points->RemovePoint(poussinTombPoint);
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(lermitagePoint));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(poussinTombPoint));

                // It's possible for the player to place these points multiple times.
                // But only the first placement elicits exclamations from Grace.
                bool alreadyPlacedPoints = mMap.zoomedIn.GetPlacedPointNearPoint(kLermitagePoint, true) != Vector2::Zero;
                if(!alreadyPlacedPoints)
                {
                    // Grace says "Wow, it intersects the meridian at the same spot as the sunrise line!"
                    gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O3H2ZBY2\", 1)");

                    // Add locked points.
                    mMap.zoomedIn.lockedPoints->AddPoint(kLermitagePoint);
                    mMap.zoomedIn.lockedPoints->AddPoint(kPoussinTombPoint);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kLermitagePoint));
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPoussinTombPoint));

                    // Place a line segment between the points.
                    mMap.zoomedIn.lines->AddLine(kLermitagePoint, kPoussinTombPoint);
                    mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kLermitagePoint),
                                                  mMap.ToZoomedOutPoint(kPoussinTombPoint));

                    // Leo is done!
                    gGameProgress.ChangeScore("e_sidney_map_poussin");
                    gGameProgress.SetFlag("Leo");
                    gGameProgress.SetFlag("PlacedTombLine");
                    SidneyUtil::UpdateLSRState();
                    justCompletedLeo = true;
                }
            }
        }

        // If we didn't finish Leo (or it was already done), check if we finished Virgo.
        if(!virgoDone && !justCompletedLeo)
        {
            // Player must place four points in the correct spots to pass Virgo.
            const Vector2 kCorner1(360.0f, 312.0f);
            const Vector2 kCorner2(578.0f, 210.0f);
            const Vector2 kCorner3(992.0f, 1084.0f);
            const Vector2 kCorner4(773.0f, 1188.0f);

            // See if placed points meet the criteria to finish Virgo.
            Vector2 point1 = mMap.zoomedIn.GetPlacedPointNearPoint(kCorner1);
            Vector2 point2 = mMap.zoomedIn.GetPlacedPointNearPoint(kCorner2);
            Vector2 point3 = mMap.zoomedIn.GetPlacedPointNearPoint(kCorner3);
            Vector2 point4 = mMap.zoomedIn.GetPlacedPointNearPoint(kCorner4);
            if(point1 != Vector2::Zero && point2 != Vector2::Zero && point3 != Vector2::Zero && point4 != Vector2::Zero)
            {
                // Says "points define 4-to-1 rectangle."
                ShowAnalyzeMessage("MapRectNote", Vector2(), HorizontalAlignment::Center);

                // Remove points placed by the player.
                mMap.zoomedIn.points->RemovePoint(point1);
                mMap.zoomedIn.points->RemovePoint(point2);
                mMap.zoomedIn.points->RemovePoint(point3);
                mMap.zoomedIn.points->RemovePoint(point4);
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point1));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point2));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point3));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point4));

                // It's possible for the player to place these points multiple times.
                // But only the first placement elicits exclamations from Grace.
                bool alreadyPlacedPoints = mMap.zoomedIn.GetPlacedPointNearPoint(kCorner1, true) != Vector2::Zero;
                if(!alreadyPlacedPoints)
                {
                    // Grace says "That matches Wilkes seismic charts!"
                    gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O3H2ZKI2\", 1)", [](const Action* action){

                        // This might end the timeblock.
                        SidneyUtil::CheckForceExitSidney307A();
                    });

                    // Add locked points.
                    mMap.zoomedIn.lockedPoints->AddPoint(kCorner1);
                    mMap.zoomedIn.lockedPoints->AddPoint(kCorner2);
                    mMap.zoomedIn.lockedPoints->AddPoint(kCorner3);
                    mMap.zoomedIn.lockedPoints->AddPoint(kCorner4);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kCorner1));
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kCorner2));
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kCorner3));
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kCorner4));

                    // Place line segments between the points.
                    mMap.zoomedIn.lines->AddLine(kCorner1, kCorner2);
                    mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kCorner1), mMap.ToZoomedOutPoint(kCorner2));

                    mMap.zoomedIn.lines->AddLine(kCorner2, kCorner3);
                    mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kCorner2), mMap.ToZoomedOutPoint(kCorner3));

                    mMap.zoomedIn.lines->AddLine(kCorner3, kCorner4);
                    mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kCorner3), mMap.ToZoomedOutPoint(kCorner4));

                    mMap.zoomedIn.lines->AddLine(kCorner4, kCorner1);
                    mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kCorner4), mMap.ToZoomedOutPoint(kCorner1));

                    // Virgo is done!
                    gGameProgress.ChangeScore("e_sidney_map_virgo");
                    gGameProgress.SetFlag("Virgo");
                    gGameProgress.SetFlag("PlacedWalls");
                    SidneyUtil::UpdateLSRState();
                }
            }
        }
    }
    else if(libraDone && !scorpioDone)
    {
        // This can only be successfully done if the player has seen the Temple of Solomon email.
        if(gGameProgress.GetFlag("OpenedTempleDiagram") && !gGameProgress.GetFlag("PlacedTempleDivisions"))
        {
            // Player must place four points in the correct spots to pass Scorpio.
            const Vector2 kPoint1(670.0f, 969.0f);
            const Vector2 kPoint2(889.0f, 866.0f);
            const Vector2 kPoint3(463.0f, 531.0f);
            const Vector2 kPoint4(680.0f, 429.0f);

            // See if placed points meet the criteria to finish Virgo.
            Vector2 point1 = mMap.zoomedIn.GetPlacedPointNearPoint(kPoint1);
            Vector2 point2 = mMap.zoomedIn.GetPlacedPointNearPoint(kPoint2);
            Vector2 point3 = mMap.zoomedIn.GetPlacedPointNearPoint(kPoint3);
            Vector2 point4 = mMap.zoomedIn.GetPlacedPointNearPoint(kPoint4);
            if(point1 != Vector2::Zero && point2 != Vector2::Zero && point3 != Vector2::Zero && point4 != Vector2::Zero)
            {
                // Remove points placed by the player.
                mMap.zoomedIn.points->RemovePoint(point1);
                mMap.zoomedIn.points->RemovePoint(point2);
                mMap.zoomedIn.points->RemovePoint(point3);
                mMap.zoomedIn.points->RemovePoint(point4);
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point1));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point2));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point3));
                mMap.zoomedOut.points->RemovePoint(mMap.ToZoomedOutPoint(point4));

                // Add locked points.
                mMap.zoomedIn.lockedPoints->AddPoint(kPoint1);
                mMap.zoomedIn.lockedPoints->AddPoint(kPoint2);
                mMap.zoomedIn.lockedPoints->AddPoint(kPoint3);
                mMap.zoomedIn.lockedPoints->AddPoint(kPoint4);
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPoint1));
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPoint2));
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPoint3));
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kPoint4));

                // Place line segments between the points to create the temple layout.
                mMap.zoomedIn.lines->AddLine(kPoint1, kPoint2);
                mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kPoint1), mMap.ToZoomedOutPoint(kPoint2));

                mMap.zoomedIn.lines->AddLine(kPoint3, kPoint4);
                mMap.zoomedOut.lines->AddLine(mMap.ToZoomedOutPoint(kPoint3), mMap.ToZoomedOutPoint(kPoint4));

                // Grace says something like "that matches the temple diagram!"
                gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O3H2ZR82\", 1)");

                // This makes progress towards Scorpio, but it doesn't complete it.
                gGameProgress.ChangeScore("e_sidney_map_temple");
                gGameProgress.SetFlag("PlacedTempleDivisions");
                didValidAnalyzeAction = true;
            }
        }
    }

    // Sagitarius functions a bit differently from other LSR steps, in that you can try to do it at any time, and the game does give a response if it's the wrong time for it.
    // (I'm not sure if this was a bug in the original game, but let's mimic that behavior).
    if(!sagitariusDone)
    {
        // The "Red Serpent" is a winding trail near the top of the map. Your goal here is to put points on the trail to mark it on the map.
        // You can place a lot of points here, but the only two required ones are at the start and end of the trail.
        // If you DO place additional points, a few optional points will also be locked in.
        const Vector2 kRedSerpentTailPoint(875.0f, 1163.0f);
        const Vector2 kRedSerpentHeadPoint(735.0f, 1365.0f);
        const Vector2 kOptionalPoint1(875.0f, 1206.0f);
        const Vector2 kOptionalPoint2(837.0f, 1238.0f);
        const Vector2 kOptionalPoint3(801.0f, 1267.0f);
        const Vector2 kOptionalPoint4(765.0f, 1301.0f);

        // See if we did the required points.
        Vector2 point1 = mMap.zoomedIn.GetPlacedPointNearPoint(kRedSerpentTailPoint);
        Vector2 point2 = mMap.zoomedIn.GetPlacedPointNearPoint(kRedSerpentHeadPoint);
        if(point1 != Vector2::Zero && point2 != Vector2::Zero)
        {
            didValidAnalyzeAction = true;

            // If you haven't completed Ophiuchus, the game allows you to place these points, but just responds with "I don't think I'm ready for that shape yet."
            // And your map points get cleared out.
            if(!ophiuchusDone)
            {
                mMap.zoomedIn.points->ClearPoints();
                mMap.zoomedOut.points->ClearPoints();
                gActionManager.ExecuteDialogueAction("02O0I27NG1", 1);
            }
            else
            {
                // Otherwise, wow, you solved Sagitarius.

                // Add required locked points.
                mMap.zoomedIn.lockedPoints->AddPoint(kRedSerpentTailPoint);
                mMap.zoomedIn.lockedPoints->AddPoint(kRedSerpentHeadPoint);
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kRedSerpentTailPoint));
                mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kRedSerpentHeadPoint));

                // If the player placed points near the optional points, those also get locked.
                Vector2 optPoint1 = mMap.zoomedIn.GetPlacedPointNearPoint(kOptionalPoint1);
                if(optPoint1 != Vector2::Zero)
                {
                    mMap.zoomedIn.lockedPoints->AddPoint(kOptionalPoint1);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kOptionalPoint1));
                }
                Vector2 optPoint2 = mMap.zoomedIn.GetPlacedPointNearPoint(kOptionalPoint2);
                if(optPoint2 != Vector2::Zero)
                {
                    mMap.zoomedIn.lockedPoints->AddPoint(kOptionalPoint2);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kOptionalPoint2));
                }
                Vector2 optPoint3 = mMap.zoomedIn.GetPlacedPointNearPoint(kOptionalPoint3);
                if(optPoint3 != Vector2::Zero)
                {
                    mMap.zoomedIn.lockedPoints->AddPoint(kOptionalPoint3);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kOptionalPoint3));
                }
                Vector2 optPoint4 = mMap.zoomedIn.GetPlacedPointNearPoint(kOptionalPoint4);
                if(optPoint4 != Vector2::Zero)
                {
                    mMap.zoomedIn.lockedPoints->AddPoint(kOptionalPoint4);
                    mMap.zoomedOut.lockedPoints->AddPoint(mMap.ToZoomedOutPoint(kOptionalPoint4));
                }

                // Remove points placed by the player after locking all needed points.
                mMap.zoomedIn.points->ClearPoints();
                mMap.zoomedOut.points->ClearPoints();

                // Play Grace dialogue.
                gActionManager.ExecuteDialogueAction("0273H2ZRS2", 1);

                // Display analyze message.
                ShowAnalyzeMessage("MapLine4Note", Vector2(), HorizontalAlignment::Center);

                // Update score and flags.
                gGameProgress.ChangeScore("e_sidney_map_saggittarius");
                gGameProgress.SetFlag("PlacedSerpent");
                gGameProgress.SetFlag("Sagittarius");
                SidneyUtil::UpdateLSRState();

                // Refresh map so serpent image appears.
                mMap.RefreshImages();
            }
        }
    }

    // If a popup was displayed, then we must have performed a valid/recognized analyze action.
    if(mAnalyzePopup->IsActive())
    {
        didValidAnalyzeAction = true;
    }

    // If you analyze the map, but there is not a more specific message to show, we always show this fallback at least.
    if(!didValidAnalyzeAction)
    {
        // If points are placed, the message is different.
        if(mMap.zoomedIn.points->GetPointsCount() > 0 || mMap.zoomedIn.lockedPoints->GetPointsCount() > 0)
        {
            // Says "unclear how to analyze those points."
            ShowAnalyzeMessage("MapIndeterminateNote", Vector2(), HorizontalAlignment::Center);
        }
        else
        {
            // Says "map is too complex to analyze - try adding points or shapes."
            ShowAnalyzeMessage("MapNoPrimitiveNote");
        }
    }

    // Analyzing always clears point entry and shape selection.
    mMap.enteringPoints = false;
    mMap.ClearShapeSelection();
}

void SidneyAnalyze::AnalyzeMap_OnUseShapePressed()
{
    mSidneyFiles->ShowShapes([this](SidneyFile* selectedFile){
        mMap.AddShape(selectedFile->name);
    });
}

void SidneyAnalyze::AnalyzeMap_OnEraseShapePressed()
{
    if(mMap.IsAnyShapeSelected())
    {
        // After completing Aries, the player has placed a Rectangle of the right size on the map, but it still needs to be rotated for Taurus.
        // At this point, the game stops you from erasing the Rectangle, since that would "uncomplete" Aries.
        bool ariesCompleted = gGameProgress.GetFlag("Aries");
        bool taurusCompleted = gGameProgress.GetFlag("Taurus");
        if(mMap.selectedRectangleIndex >= 0 && (ariesCompleted && !taurusCompleted))
        {
            // Grace says "I think that's right - I don't want to erase it."
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27LN1\", 1)");
        }
        else
        {
            mMap.EraseSelectedShape();
        }
    }
    else
    {
        ShowAnalyzeMessage("NoShapeNote", Vector2(), HorizontalAlignment::Center);
    }
}

void SidneyAnalyze::AnalyzeMap_OnEnterPointsPressed()
{
    // Not allowed to place points until you've progressed far enough in the story for Grace to have something to plot.
    if(gGameProgress.GetTimeblock() == Timeblock("205P"))
    {
        // The game wants you to pick up these items before it thinks you have enough info to place points.
        if(!gInventoryManager.HasInventoryItem("Church_Pamphlet") || !gInventoryManager.HasInventoryItem("LSR"))
        {
            gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O8O2ZPI1\", 1)");
            return;
        }
    }

    // Otherwise, we can place points.
    AnalyzeMap_SetStatusText(SidneyUtil::GetAnalyzeLocalizer().GetText("EnterPointsNote"));
    mMap.enteringPoints = true;
}

void SidneyAnalyze::AnalyzeMap_OnClearPointsPressed()
{
    mMap.enteringPoints = false;
    mMap.zoomedIn.points->ClearPoints();
    mMap.zoomedOut.points->ClearPoints();
}

void SidneyAnalyze::AnalyzeMap_OnDrawGridPressed()
{
    if(mMap.zoomedIn.grids->GetCount() == 0)
    {
        // Figure out what we want the grid to fill (whole screen or a shape).
        std::vector<std::string> gridFillChoices = {
            SidneyUtil::GetAnalyzeLocalizer().GetText("GridFillScreen"),
            SidneyUtil::GetAnalyzeLocalizer().GetText("GridFillShape"),
            SidneyUtil::GetAnalyzeLocalizer().GetText("GridCancel")
        };
        mSidneyFiles->ShowCustom(SidneyUtil::GetAnalyzeLocalizer().GetText("GridList"), gridFillChoices, [this](size_t fillChoiceIdx){
            // Early out if user chose "cancel" option.
            if(fillChoiceIdx == 2) { return; }

            // Determine whether we want to fill the screen or a shape.
            bool fillShape = (fillChoiceIdx == 1);

            // Grace usually doesn't want to fill a shape.
            // There's only one time (during Gemini) that she'll let you proceed past this point.
            if(fillShape)
            {
                bool taurusDone = gGameProgress.GetFlag("Taurus");
                bool geminiDone = gGameProgress.GetFlag("Gemini");
                bool workingOnGemini = (taurusDone && !geminiDone);
                if(!workingOnGemini)
                {
                    // "I don't think a grid will help me here."
                    gActionManager.ExecuteSheepAction("wait StartDialogue(\"02OD32ZNF1\", 1)");
                    return;
                }
            }

            // Figure out what size grid to use.
            std::vector<std::string> gridSizeChoices = {
                SidneyUtil::GetAnalyzeLocalizer().GetText("Grid2"),
                SidneyUtil::GetAnalyzeLocalizer().GetText("Grid4"),
                SidneyUtil::GetAnalyzeLocalizer().GetText("Grid8"),
                SidneyUtil::GetAnalyzeLocalizer().GetText("Grid12"),
                SidneyUtil::GetAnalyzeLocalizer().GetText("Grid16"),
                SidneyUtil::GetAnalyzeLocalizer().GetText("GridCancel")
            };
            mSidneyFiles->ShowCustom(SidneyUtil::GetAnalyzeLocalizer().GetText("GridList"), gridSizeChoices, [this, fillShape](size_t sizeChoiceIdx){
                // Early out if user chose "cancel" option.
                if(sizeChoiceIdx == 5) { return; }

                // Otherwise, figure out size.
                uint8_t gridSize = 2;
                switch(sizeChoiceIdx)
                {
                case 0:
                    gridSize = 2;
                    break;
                case 1:
                    gridSize = 4;
                    break;
                case 2:
                    gridSize = 8;
                    break;
                case 3:
                    gridSize = 12;
                    break;
                case 4:
                    gridSize = 16;
                    break;
                }

                // Draw the grid.
                mMap.DrawGrid(gridSize, fillShape);

                // Usually, drawing a grid is NOT the answer!
                // In fact, the only time it actually helps is during Gemini if you fill the shape.
                bool taurusDone = gGameProgress.GetFlag("Taurus");
                bool geminiDone = gGameProgress.GetFlag("Gemini");
                bool workingOnGemini = (taurusDone && !geminiDone);
                if(workingOnGemini && fillShape)
                {
                    // Bingo!
                    if(gridSize == 8)
                    {
                        // Lock the placed grid.
                        mMap.LockGrid();

                        // Grace says "That's it! That's the chessboard."
                        gActionManager.ExecuteSheepAction("wait StartDialogue(\"02OCL2ZJL1\", 1)", [this](const Action* action){

                            // Gemini AND Cancer completed in one fell swoop.
                            gGameProgress.SetFlag("Gemini");
                            gGameProgress.SetFlag("Cancer");
                            gGameProgress.SetFlag("PlacedGrid");
                            gGameProgress.ChangeScore("e_sidney_map_gemini");
                            SidneyUtil::UpdateLSRState();

                            // Hide Sidney, but not using the normal "Hide", which causes a location change we don't want in this case.
                            mSidney->SetActive(false);

                            // Ok, this is the end of the 205P timeblock. All conditions are set to move on.
                            // We should warp to the hallway now (where the next timeblock begins).
                            // The timeblock complete script will move to the next timeblock.
                            gLocationManager.ChangeLocation("HAL");
                        });
                    }
                    else
                    {
                        // Grace says "I'm not sure about the size of the grid."
                        gActionManager.ExecuteSheepAction("wait StartDialogue(\"02OD32ZGW1\", 1)");
                    }
                }
                else
                {
                    // Grace says "Hmm, not sure about that."
                    gActionManager.ExecuteSheepAction("wait StartDialogue(\"02O0I27GT1\", 1)");
                }
            });
        });
    }
    else
    {
        // Only allowed to place one grid at a time. This message says "erase other grid first."
        ShowAnalyzeMessage("GridDispNote");
    }
}

void SidneyAnalyze::AnalyzeMap_OnEraseGridPressed()
{
    if(mMap.zoomedIn.grids->GetCount() > 0)
    {
        mMap.ClearGrid();
    }
    else
    {
        // Display an error if there is no grid to erase.
        ShowAnalyzeMessage("NoGridEraseNote");
    }
}

void SidneyAnalyze::AnalyzeMap_SetStatusText(const std::string& text, float duration)
{
    mMapStatusLabel->SetEnabled(true);
    mMapStatusLabel->SetText(text);
    mMapStatusLabelTimer = duration;
}

void SidneyAnalyze::AnalyzeMap_SetPointStatusText(const std::string& baseMessage, const Vector2& zoomedInMapPos)
{
    AnalyzeMap_SetStatusText(StringUtil::Format(SidneyUtil::GetAnalyzeLocalizer().GetText(baseMessage).c_str(),
                                                mMap.GetPointLatLongText(zoomedInMapPos).c_str()), 0.0f);
}