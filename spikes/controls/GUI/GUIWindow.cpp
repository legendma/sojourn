#include "pch.h"
#include "GUIWindow.h"

GUIWindow::GUIWindow( IEngine &engine, GUIWindow *parent, GUIWindowId id, const GUIPoint &position, const GUISize &size ) :
    engine( engine ),
    parent( parent ),
    id( id ),
    position( position ),
    size( size )
{
}

GUIWindow::~GUIWindow()
{
    children.clear();
}

void GUIWindow::AddChild( GUIWindow *child )
{
    children.push_back( std::shared_ptr<GUIWindow>( child ) );
}
