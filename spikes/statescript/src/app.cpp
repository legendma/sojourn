#include "pch.hpp"

#include "app.hpp"

using namespace math;

auto const SAVE_PROGRAM_POPUP = "save_program_popup";

static const ImVec2 ToImVec2( const Vec2 &vec ) { return ImVec2( vec.v.x, vec.v.y ); }
static const Vec2 FromImVec2( const ImVec2& vec ) { return Vec2( vec.x, vec.y ); }

void App::AddNewStateScriptProgram()
{
auto &new_program = open_programs[ next_statescript_program_id ];
new_program.id = next_statescript_program_id;
std::ostringstream os;
os << "New Program ";
os << next_statescript_program_id;
new_program.name = os.str();
new_program.is_dirty = true;

AddNewNodeToProgram( assets::StateScriptNodeName::ENTRY_POINT, new_program );

++next_statescript_program_id;
selected_program_id = new_program.id;
}

void App::BuildNodeRenderable( StateScriptNodeRenderable &node, assets::StateScriptNodeId asset, std::string &name, Vec2 &position, Vec4 &color, const assets::StateScriptProgram &program )
{
static const auto DISPLAY_NAME_MARGINS = Vec2( 5.0f, 3.0f );
static const auto PLUG_LABEL_MARGINS = Vec2( 4.0f, 3.0f );
const auto PLUG_RADIUS = gui::CalcTextSize( "O" ).x;
const auto BETWEEN_IN_AND_OUT_LABELS_H_MARGIN = 20.0f;

node.asset = asset;
node.color = color;
node.name = name;
node.abb.position = position;

std::vector<assets::StateScriptPlugId> plug_ids;
assets::StateScriptFactory::EnumerateNodePlugs( node.asset, program, plug_ids );
for( auto plug_id : plug_ids )
{
    auto &plug = assets::StateScriptFactory::GetPlugById( plug_id, program );
    auto render_plugs = std::ref( node.in_plugs );

    if( assets::StateScriptFactory::IsOutputPlug( plug.name ) )
        {
        render_plugs = std::ref( node.out_plugs );
        }

    render_plugs.get().emplace_back();
    auto &render_plug = render_plugs.get().back();
    render_plug.asset = plug_id;
    render_plug.label = assets::StateScriptFactory::NodePlugDisplayNames[ (int)plug.name ];
    render_plug.jack_radius = PLUG_RADIUS;
}

auto name_abb = ABB2( Pos2(), Vec2::Scale( 2.0f, DISPLAY_NAME_MARGINS ) );
auto name_size = FromImVec2( gui::CalcTextSize( node.name.c_str() ) );
name_abb.extent = Vec2::Add( name_abb.extent, name_size );

auto grow_plug_abb = []( std::vector<App::StateScriptNodeRenderable::Plug> &plugs, ABB2 &abb )
    {
    for( auto &plug_render : plugs )
        {
        auto text_size = gui::CalcTextSize( plug_render.label.c_str() );
        abb.extent.v.x = std::max( abb.extent.v.x, 2.0f * PLUG_LABEL_MARGINS.v.x + text_size.x );
        abb.extent.v.y += 2.0f * PLUG_LABEL_MARGINS.v.y + text_size.y;
        }
    };

auto ins_abb = ABB2( Pos2( 0.0f, name_abb.extent.v.y), Vec2() );
auto outs_abb = ins_abb;
grow_plug_abb( node.in_plugs, ins_abb );
grow_plug_abb( node.out_plugs, outs_abb );
outs_abb.position.v.x = ins_abb.position.v.x + ins_abb.extent.v.x;

auto plugs_abb = ABB2::Union( ins_abb, outs_abb );
plugs_abb.extent.v.x += BETWEEN_IN_AND_OUT_LABELS_H_MARGIN;

node.abb.extent = ABB2::Union( name_abb, plugs_abb ).extent;
node.name_position = Pos2::Add( node.abb.position, Vec2::Scale( 0.5f, Vec2( node.abb.extent.v.x - name_size.v.x, 0.0f ) ) );
node.name_position.v.y += DISPLAY_NAME_MARGINS.v.y;

int i = 0;
for( auto it = node.in_plugs.rbegin(); it != node.in_plugs.rend(); it++, i++ )
    {
    auto text_size = FromImVec2( gui::CalcTextSize( it->label.c_str() ) );
    auto text_size_and_margins = Vec2::Add( text_size, PLUG_LABEL_MARGINS );
    auto y = node.abb.extent.v.y - ( (float)i + 0.5f ) * text_size_and_margins.v.y;
    auto x = 0.0f;
    it->jack_position = Pos2( x, y );
    it->label_position = Pos2( x + it->jack_radius + PLUG_LABEL_MARGINS.v.x, y - 0.5f * text_size.v.y );
    }

i = 0;
for( auto it = node.out_plugs.rbegin(); it != node.out_plugs.rend(); it++, i++ )
    {
    auto text_size = FromImVec2( gui::CalcTextSize( it->label.c_str() ) );
    auto text_size_and_margins = Vec2::Add( text_size, PLUG_LABEL_MARGINS );
    auto y = node.abb.extent.v.y - ((float)i + 0.5f) * text_size_and_margins.v.y;
    auto x = node.abb.extent.v.x;
    it->jack_position = Pos2( x, y );
    it->label_position = Pos2( x - it->jack_radius - PLUG_LABEL_MARGINS.v.x - text_size.v.x, y - 0.5f * text_size.v.y );
    }

}


void App::AddNewNodeToProgram( const assets::StateScriptNodeName the_type, StateScriptProgramRecord &program )
{
program.nodes.emplace_back();
auto &render = program.nodes.back();

auto asset = assets::StateScriptFactory::AddNodeToProgram( the_type, program.asset );
auto color = assets::StateScriptFactory::NodeDisplayColors[ (int)the_type ];
auto display_name = assets::StateScriptFactory::NodeDisplayNames[(int)the_type];
auto position = Pos2( 0.0f, 0.0f );

BuildNodeRenderable( render, asset, display_name, position, color, program.asset );

program.node_draw_order.push_back( render.asset );
}

void App::CloseProgram( StateScriptProgramId program_id, StateScriptProgramIds &pending )
{
auto &program = open_programs[ program_id ];
if( program.is_dirty )
    {
    if( AskUserToSaveProgram( program_id ) )
        {
        pending.push_back( program_id );
        selected_program_id = 0;
        }
    }
}


/* Returns false if program can now be closed */
bool App::AskUserToSaveProgram( StateScriptProgramId program_id )
{
auto &program = open_programs[ program_id ];
auto close_program = false;
if( gui::BeginPopup( SAVE_PROGRAM_POPUP ) )
    {
    std::ostringstream os;
    os << "Save ";
    os << program.name;
    os << "?";
    gui::NewLine();
    gui::Text( os.str().c_str() );
    gui::NewLine();

    if( gui::Button( "Yes" ) )
        {
        SaveProgram( program_id );
        gui::CloseCurrentPopup();
        close_program = true;
        }

    gui::SameLine();
    if( gui::Button( "No" ) )
        {
        gui::CloseCurrentPopup();
        close_program = true;
        }

    gui::SameLine();
    if( gui::Button( "Cancel" ) )
        {
        gui::CloseCurrentPopup();
        }

    gui::EndPopup();
    }

return( close_program );
}


void App::SaveProgram( StateScriptProgramId program_id )
{
}


void App::ShowMainMenuStateScript()
{
if( gui::BeginMenu( "Insert" ) )
    {
    if( gui::MenuItem( "New Program" ) ) { AddNewStateScriptProgram(); }
    if( selected_program_id )
        {
        //if( gui::MenuItem( "New Module") ) { AddNewModuleToSelectedProgram(); }
        }

    gui::EndMenu();
    }
}


void App::ShowStateScriptPrograms()
{
StateScriptProgramIds pending_close;
for( auto &it : open_programs )
    {
    auto &program = it.second;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    if( program.is_dirty )
        {
        window_flags |= ImGuiWindowFlags_UnsavedDocument;
        }

    bool p_open;
    const ImGuiViewport* viewport = gui::GetMainViewport();
    gui::SetNextWindowPos( viewport->WorkPos );
    gui::SetNextWindowSize( viewport->WorkSize );
    gui::Begin( program.name.c_str(), &p_open, window_flags );
    if( gui::IsItemActive() )
        {
        selected_program_id = program.id;
        }

    if( !p_open )
        {
        gui::OpenPopup( SAVE_PROGRAM_POPUP );
        }

    CloseProgram( program.id, pending_close );

    gui::BeginChild( "Details Pane", ImVec2( 250, 0 ), true );

    gui::PushID( (void*)&program.new_node_selection.filter_select_idx );
    auto label = assets::StateScriptFactory::NodeFilterDisplayNames[ program.new_node_selection.filter_select_idx ].c_str();
    if( gui::BeginCombo( "##", label ) )
        {
        for( auto filter = 0; filter < (int)assets::StateScriptNodeFilterId::CNT; filter++ )
            {
            auto is_selected = filter == program.new_node_selection.filter_select_idx;
            if( gui::Selectable( assets::StateScriptFactory::NodeFilterDisplayNames[ filter ].c_str(), is_selected ) )
                {
                program.new_node_selection.filter_select_idx = filter;
                program.new_node_selection.node_id_select_idx = 0;
                }

            if( is_selected )
                {
                gui::SetItemDefaultFocus();
                }
            }

        gui::EndCombo();
        }
    gui::PopID();

    std::vector<assets::StateScriptNodeName> selection_node_names;
    switch( program.new_node_selection.filter_select_idx )
        {
        case (int)assets::StateScriptNodeFilterId::SPECIAL:
            for( auto name : assets::SPECIAL_NODES_FILTER )
                {
                selection_node_names.push_back( name );
                }
            break;

        case (int)assets::StateScriptNodeFilterId::CONDITIONAL:
            for( auto name : assets::CONDITIONAL_NODES_FILTER )
                {
                selection_node_names.push_back( name );
                }
            break;

        case (int)assets::StateScriptNodeFilterId::STATE:
            for( auto name : assets::STATE_NODES_FILTER )
                {
                selection_node_names.push_back( name );
                }
            break;
        }

    auto prev_node_name = selection_node_names[ program.new_node_selection.node_id_select_idx ];
    gui::PushID( (void*)&program.new_node_selection.node_id_select_idx );
    label = assets::StateScriptFactory::NodeDisplayNames[ (int)prev_node_name].c_str();
    if( gui::BeginCombo( "##", label ) )
        {
        for( auto name_idx = 0; name_idx < selection_node_names.size(); name_idx++ )
            {
            auto this_node_name = selection_node_names[ name_idx ];
            label = assets::StateScriptFactory::NodeDisplayNames[ (int)this_node_name ].c_str();
            auto is_selected = program.new_node_selection.node_id_select_idx == name_idx;
            if( gui::Selectable( label, is_selected ) )
                {
                program.new_node_selection.node_id_select_idx = name_idx;
                }

            if( is_selected )
                {
                gui::SetItemDefaultFocus();
                }
            }

        gui::EndCombo();
        }
    gui::PopID();

    gui::SameLine();
    if( gui::Button( "Add" ) )
        {
        auto new_node_to_add = selection_node_names[ program.new_node_selection.node_id_select_idx ];
        AddNewNodeToProgram( new_node_to_add, program );
        }

    gui::Separator();
    gui::NewLine();

    gui::Text( "Program Graph:" );

    if( gui::BeginTable( "program_table", 2, /*ImGuiTableFlags_BordersOuter |*/ ImGuiTableFlags_Resizable ) )
        {
        gui::TableNextRow();
        gui::TableSetColumnIndex( 0 );
        if( gui::TreeNode( "Nodes" ) )
            {
            for( auto node_id : program.node_draw_order )
                {
                auto &node = assets::StateScriptFactory::GetNodeById( node_id, program.asset );
                gui::TableNextRow();
                gui::TableSetColumnIndex( 0 );
                auto node_name = assets::StateScriptFactory::NodeDisplayNames[(int)node.name].c_str();
                if( gui::TreeNodeEx( node_name, 0, "%s_%d", node_name, node.node_id ) )
                    {
                    auto &plugs = program.asset.plugs;
                    std::vector<assets::StateScriptPlugId> plug_ids;
                    assets::StateScriptFactory::EnumerateNodePlugs( node.node_id, program.asset, plug_ids );
                    for( auto plug_id : plug_ids )
                        {
                        auto &plug = assets::StateScriptFactory::GetPlugById( plug_id, program.asset );

                        gui::TableNextRow();
                        gui::TableSetColumnIndex( 0 );
                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
                        auto plug_name = assets::StateScriptFactory::NodePlugDisplayNames[ (int)plug.name ].c_str();
                        gui::TreeNodeEx( plug_name, flags, "%s_%d", plug_name, plug.plug_id );

                        gui::TableNextRow();
                        gui::TableSetColumnIndex( 0 );
                        gui::TreeNodeEx( "plug_connections", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Connections:" );
                        gui::TableSetColumnIndex( 1 );

                        gui::NextColumn();
                        }
                    gui::TreePop();

                    }

                }

            gui::TreePop();

            }

        gui::EndTable();
        }
    //for( int i = 0; i < 100; i++ )
    //{
    //    // FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
    //    char label[128];
    //    sprintf( label, "MyObject %d", i );
    //    if( ImGui::Selectable( label, selected == i ) )
    //        selected = i;
    //}
    gui::EndChild();
    gui::SameLine();

    gui::BeginChild( "Build View", ImVec2( 0, 0 ), true );

    const ImVec2 view_top_left = gui::GetCursorScreenPos();
    ImVec2 view_extent = gui::GetContentRegionAvail();
    if( view_extent.x < 50.0f ) view_extent.x = 50.0f;
    if( view_extent.y < 50.0f ) view_extent.y = 50.0f;
    const ImVec2 canvas_bottom_right = ImVec2( view_top_left.x + view_extent.x, view_top_left.y + view_extent.y );

    gui::InvisibleButton( "view_interactions", view_extent, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight );
    const bool view_hovered = gui::IsItemHovered();
    const bool view_pressed = gui::IsItemActive();
    const Vec2 view_origin = Vec2::Sub( FromImVec2( view_top_left ), program.view_scroll );
    const Vec2 mouse_in_view_space = Vec2( gui::GetMousePos().x - view_origin.v.x, gui::GetMousePos().y - view_origin.v.y );
    
    StateScriptRenderNodes::iterator clicked_node;
    if( view_hovered
     && gui::IsMouseClicked( ImGuiMouseButton_Left ) )
        {
        program.selected_node = -1;
        if( GetClickedNode( mouse_in_view_space, program, clicked_node ) )
            {
            program.selected_node = clicked_node->asset;
            BringNodeToFront( program.selected_node, program );
            }
        }

    if( view_pressed
     && gui::IsMouseDragging( ImGuiMouseButton_Left )
     && GetClickedNode( mouse_in_view_space, program, clicked_node ) )
        {
        auto drag_delta = gui::GetMouseDragDelta();
        auto this_drag_delta = Vec2::Sub( FromImVec2( drag_delta ), program.last_drag_delta );
        clicked_node->abb.position = Vec2::Add( clicked_node->abb.position, this_drag_delta );
        }
    
    if( view_hovered )
        {
        program.last_drag_delta = Vec2( 0.0f, 0.0f );
        if( gui::IsMouseDragging( ImGuiMouseButton_Left ) )
            {
            program.last_drag_delta = FromImVec2( gui::GetMouseDragDelta() );
            }
        }

    for( auto node_draw : program.node_draw_order )
        {
        auto &node_record = std::find_if( program.nodes.begin(), program.nodes.end(), [node_draw]( const StateScriptNodeRenderable &r ){ return r.asset == node_draw; } );
        DrawStateScriptNode( view_origin, node_record->asset == program.selected_node, *node_record, program.asset );
        }

    gui::EndChild();

    gui::End();
    }

for( auto program_id : pending_close )
    {
    open_programs.erase( program_id );
    }

}


bool App::ShowWindow()
{
bool done = false;

if( current_workflow == WorkflowId::WORKFLOW_ID_STATESCRIPT )
    {
    ShowStateScriptPrograms();
    }

if( gui::BeginMainMenuBar() )
    {
    if( gui::BeginMenu( "Start" ) )
        {
        auto needs_separator = false;
        if( current_workflow == WorkflowId::WORKFLOW_ID_STATESCRIPT )
            {
            needs_separator = true;
            if( gui::MenuItem( "Load Program" ) )
                {
            
                }
             }

        if( needs_separator )
            {
            gui::Separator();
            }

        if( gui::MenuItem( "Quit" ) ) { done = true; }

        gui::EndMenu();
        }

    if( gui::BeginMenu( "Workflow" ) )
        {
        gui::MenuItem( "Script Edit", NULL, current_workflow == WorkflowId::WORKFLOW_ID_STATESCRIPT );
        gui::EndMenu();
        }

    if( current_workflow == WorkflowId::WORKFLOW_ID_STATESCRIPT )
    {
        ShowMainMenuStateScript();
    }

    gui::EndMainMenuBar();
    }

    return( !done );
}


void App::DrawStateScriptNode( const Vec2 origin, const bool selected, const StateScriptNodeRenderable &node, const assets::StateScriptProgram &program )
{
auto draw_list = gui::GetWindowDrawList();
auto abb_world = node.abb;
abb_world.position = Vec2::Add( node.abb.position, origin );
auto node_color = ImColor( node.color.v.x, node.color.v.y, node.color.v.z, node.color.v.w );

/* background */
auto rect_upper_left = Pos2::Add( node.abb.position, origin );
auto rect_lower_right = Pos2::Add( rect_upper_left, node.abb.extent );
draw_list->AddRectFilled( ToImVec2( rect_upper_left ), ToImVec2( rect_lower_right ), node_color );
if( selected )
    {
    draw_list->AddRect( ToImVec2( rect_upper_left ), ToImVec2( rect_lower_right ), ImColor( 1.0f, 1.0f, 1.0f, 1.0f ) );
    }

/* name */
auto name_extent = FromImVec2( gui::CalcTextSize( node.name.c_str() ) );
auto name_upper_left = Vec2::Add( rect_upper_left, node.name_position );
draw_list->AddText( ToImVec2( name_upper_left ), ImColor( 1.0f, 1.0f, 1.0f, 1.0f ), node.name.c_str() );

/* plugs */
auto draw_plugs = [draw_list, node_color, rect_upper_left]( const std::vector<StateScriptNodeRenderable::Plug> &plugs )
    {
    for( auto &in_plug : plugs )
        {
        auto jack_position = Pos2::Add( in_plug.jack_position, rect_upper_left );
        draw_list->AddCircleFilled( ToImVec2( jack_position ), in_plug.jack_radius, node_color );
        draw_list->AddCircle( ToImVec2( jack_position ), in_plug.jack_radius, ImColor( 1.0f, 1.0f, 1.0f, 0.8f ) );

        auto text_position = Pos2::Add( in_plug.label_position, rect_upper_left );
        draw_list->AddText( ToImVec2( text_position ), ImColor( 1.0f, 1.0f, 1.0f, 1.0f ), in_plug.label.c_str() );
        }
    };

draw_plugs( node.in_plugs );
draw_plugs( node.out_plugs );

}

void App::BringNodeToFront( const assets::StateScriptNodeId node, StateScriptProgramRecord &program )
{
auto found = std::find_if( program.node_draw_order.begin(), program.node_draw_order.end(), [node]( assets::StateScriptNodeId search ) { return search == node; } );
if( found == program.node_draw_order.end() ) return;

program.node_draw_order.erase( found );
program.node_draw_order.push_back( node );
}

void App::SendNodeToBack( const assets::StateScriptNodeId node, StateScriptProgramRecord &program )
{
auto found = std::find_if( program.node_draw_order.begin(), program.node_draw_order.end(), [node]( assets::StateScriptNodeId search ) { return search == node; } );
if( found == program.node_draw_order.end() ) return;

program.node_draw_order.erase( found );
program.node_draw_order.push_front( node );
}

bool App::GetClickedNode( const Vec2 view_click, const StateScriptProgramRecord &program, StateScriptRenderNodes::iterator &out_node )
{
auto &non_const = const_cast<StateScriptRenderNodes&>( program.nodes );
for( auto index = program.node_draw_order.rbegin(); index != program.node_draw_order.rend(); index++ ) 
    {
    out_node = std::find_if( non_const.begin(), non_const.end(), [index]( const StateScriptNodeRenderable&r ){ return r.asset == *index; } );
    auto &node = *out_node;
    Vec2 top_left = node.abb.position;
    Vec2 bottom_right = ABB2::BottomRight( node.abb );
    if( view_click.v.x <  top_left.v.x
     || view_click.v.y <  top_left.v.y
     || view_click.v.x >= bottom_right.v.x
     || view_click.v.y >= bottom_right.v.y ) continue;

    return true;
    }

return false;

}

