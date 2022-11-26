
#include "statescript.hpp"

using namespace assets;

const std::array<std::string, (size_t)StateScriptNodeName::STATESCRIPT_NODE_NAME_CNT> StateScriptFactory::NodeDisplayNames =
    {
    "Invalid",
    "EntryPoint"
    };

const std::array<Vec4, (size_t)StateScriptNodeName::STATESCRIPT_NODE_NAME_CNT> StateScriptFactory::NodeDisplayColors =
    {
    Vec4( 0.0f, 0.0f, 0.0f, 0.0f ),
    Vec4( 1.0f, 0.5f, 0.5f, 1.0f )
    };

const std::array<std::string, (size_t)StateScriptPlugName::STATESCRIPT_PLUG_NAME_CNT> StateScriptFactory::NodePlugDisplayNames =
    {
    "Invalid",
    "In",
    "IActivate",
    "Out"
    };

void assets::StateScriptFactory::SaveProgram( const StateScriptProgram& program, const std::string filename )
{
}

bool assets::StateScriptFactory::LoadProgram( const std::string filename, StateScriptProgram& program )
{
    return false;
}

StateScriptNodeRecord & assets::StateScriptFactory::GetNodeById( const StateScriptNodeId id, StateScriptProgram &program )
{
return *std::find_if( program.nodes.begin(), program.nodes.end(), [id]( const StateScriptNodeRecord &record ) { return record.node_id == id; } );
}

StateScriptPlugRecord& assets::StateScriptFactory::GetPlugById( const StateScriptPlugId id, StateScriptProgram &program )
{
return *std::find_if( program.plugs.begin(), program.plugs.end(), [id]( const StateScriptPlugRecord& record ) { return record.plug_id == id; } );
}

void assets::StateScriptFactory::GetPlugsForNode( const StateScriptNodeId node, const StateScriptProgram &program, std::vector<StateScriptPlugId>& out )
{
out.clear();
for( auto i = 0; i < program.plugs.size(); i++ )
    {
    if( program.plugs[ i ].owner_node_id == node ) out.push_back( program.plugs[ i ].plug_id );
    }
}

StateScriptNodeId assets::StateScriptFactory::AddNodeToProgram( const StateScriptNodeName the_type, StateScriptProgram& program )
{
program.nodes.emplace_back();
auto &node = program.nodes.back();
node.node_id = GenerateNewUid();
node.name = the_type;

switch( the_type )
    {
    case StateScriptNodeName::STATESCRIPT_NODE_NAME_ENTRY_POINT:
        {
        AddPlugToNode( node.node_id, StateScriptPlugName::STATESCRIPT_PLUG_NAME_OUT_DEFAULT, program );
        }
        break;
    }

return node.node_id;
}

StateScriptPlugId assets::StateScriptFactory::AddPlugToNode( const StateScriptNodeId node, const StateScriptPlugName the_type, StateScriptProgram& program )
{
program.plugs.emplace_back();
auto &plug = program.plugs.back();
plug.name = the_type;
plug.owner_node_id = node;
plug.plug_id = GenerateNewUid();

return plug.plug_id;
}

uint32_t assets::StateScriptFactory::GenerateNewUid()
{
static uint32_t next_uid = 0;
return next_uid++;
}