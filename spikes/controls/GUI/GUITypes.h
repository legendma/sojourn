#pragma once

typedef int GUIWindowId;

struct GUIPoint
{
    int x = -1;
    int y = -1;

    GUIPoint() = default;
    GUIPoint( int x, int y ) : x( x ), y( y ) {}
};

static const GUIPoint GUIDefaultPosition;

struct GUISize
{
    int width = -1;
    int height = -1;

    GUISize() = default;
    GUISize( int width, int height ) : width( width ), height( height ) {}
};

static const GUISize GUIDefaultSize;
