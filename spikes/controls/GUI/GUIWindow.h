#pragma once
#include "IEngine.h"
#include "GUITypes.h"

class GUIWindow
{
public:
    GUIWindow( IEngine &engine, GUIWindow *parent, GUIWindowId id, const GUIPoint &position = GUIDefaultPosition, const GUISize &size = GUIDefaultSize );
    virtual ~GUIWindow();

    virtual void AddChild( GUIWindow *child );
    //virtual void Update();

protected:
    IEngine &engine;
    std::shared_ptr<GUIWindow> parent;
    GUIWindowId id;
    GUIPoint position;
    GUISize size;
    std::vector<std::shared_ptr<GUIWindow>> children;

};

