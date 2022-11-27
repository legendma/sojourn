#include "pch.hpp"

#include "app.hpp"

auto const SAVE_PROGRAM_POPUP = "save_program_popup";

void App::AddNewStateScriptProgram()
{
auto &new_program = open_programs[ next_statescript_program_id ];
new_program.id = next_statescript_program_id;
std::ostringstream os;
os << "New Program ";
os << next_statescript_program_id;
new_program.name = os.str();
new_program.is_dirty = true;

AddNewNodeToProgram( assets::StateScriptNodeName::STATESCRIPT_NODE_NAME_ENTRY_POINT, new_program );

++next_statescript_program_id;
selected_program_id = new_program.id;
}


void App::AddNewNodeToProgram( const assets::StateScriptNodeName the_type, StateScriptProgramRecord& program )
{
program.nodes.emplace_back();
auto &node_render = program.nodes.back();

node_render.asset = assets::StateScriptFactory::AddNodeToProgram( the_type, program.asset );
node_render.color = assets::StateScriptFactory::NodeDisplayColors[ (int)the_type ];
node_render.display_name = assets::StateScriptFactory::NodeDisplayNames[ (int)the_type ];
node_render.position = Pos2( 0.0f, 0.0f );
node_render.extent   = Vec2( 100.0f, 200.0f );

program.node_draw_order.push_back( node_render.asset );
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

    int new_node_classify = 0;

    gui::BeginChild( "Details Pane", ImVec2( 250, 0 ), true );

    gui::Combo( "##", &new_node_classify, "New Conditional\0" );
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
                    assets::StateScriptFactory::GetPlugsForNode( node.node_id, program.asset, plug_ids );
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
    for( auto node_render : program.nodes )
        {
        DrawStateScriptNode( node_render, program.asset );
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
        gui::MenuItem( "StateScript", NULL, current_workflow == WorkflowId::WORKFLOW_ID_STATESCRIPT );
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


void App::DrawStateScriptNode( const StateScriptNodeRenderable &node, const assets::StateScriptProgram &program )
{
auto draw_list = gui::GetWindowDrawList();
auto p = gui::GetCursorScreenPos();
p.x += node.position.v.x;
p.y += node.position.v.y;
auto node_color = ImColor( node.color.v.x, node.color.v.y, node.color.v.z, node.color.v.w );

/* background */
draw_list->AddRectFilled( ImVec2( p.x, p.y ), ImVec2( p.x + node.extent.v.x, p.y + node.extent.v.y ), node_color );

/* name */
auto text_extent = gui::CalcTextSize( node.display_name.c_str() );
draw_list->AddText( ImVec2( p.x + 0.5f * node.extent.v.x - 0.5f * text_extent.x, p.y + 0.5f * text_extent.y ),
                    ImColor( 1.0f, 1.0f, 1.0f, 1.0f ), node.display_name.c_str() );

/* plugs */
auto &asset = assets::StateScriptFactory::GetNodeById( node.asset, program );
std::vector<assets::StateScriptPlugId> plugs;
assets::StateScriptFactory::GetPlugsForNode( asset.node_id, program, plugs );
int in_count = 0, out_count = 0;
for( auto plug_id : plugs )
    {
    auto &plug = assets::StateScriptFactory::GetPlugById( plug_id, program );
    if( assets::StateScriptFactory::IsInputPlug(  plug.name ) ) { in_count++;  }
    if( assets::StateScriptFactory::IsOutputPlug( plug.name ) ) { out_count++; }
    }

auto between_in_plugs  = node.extent.v.x / ( in_count  + 1 );
auto between_out_plugs = node.extent.v.y / ( out_count + 1 );
const float PLUG_RADIUS = gui::CalcTextSize( "O" ).x;

in_count = out_count = 0;
for( auto plug_id : plugs )
    {
    auto plug_pos = p;
    auto &plug = assets::StateScriptFactory::GetPlugById( plug_id, program );
    float text_h_just = 0.0f;

    auto plug_name = assets::StateScriptFactory::NodePlugDisplayNames[(int)plug.name].c_str();
    auto plug_name_size = gui::CalcTextSize( plug_name );
    if( assets::StateScriptFactory::IsInputPlug( plug.name ) )
        {
        plug_pos.y += between_in_plugs * (float)++in_count;
        text_h_just = gui::CalcTextSize( "WW" ).x;
        }

    if( assets::StateScriptFactory::IsOutputPlug( plug.name ) )
        {
        plug_pos.x += node.extent.v.x;
        plug_pos.y += between_out_plugs * (float)++out_count;
        text_h_just = -gui::CalcTextSize( "WW" ).x - plug_name_size.x;
        }

    draw_list->AddCircleFilled( plug_pos, PLUG_RADIUS, node_color );
    draw_list->AddCircle( plug_pos, PLUG_RADIUS, ImColor( 1.0f, 1.0f, 1.0f, 0.8f ) );
    draw_list->AddText( ImVec2( plug_pos.x + text_h_just, plug_pos.y - 0.5f * plug_name_size.y ), ImColor( 1.0f, 1.0f, 1.0f, 1.0f ), plug_name );
    }

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
program.node_draw_order.insert( program.node_draw_order.begin(), 1, node );
}
