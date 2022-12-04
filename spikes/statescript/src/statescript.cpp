
#include "statescript.hpp"

using namespace assets;

const std::array<std::string, (size_t)StateScriptNodeName::CNT> StateScriptFactory::NodeDisplayNames =
    {
    "Invalid",
    "EntryPoint",
    "Dummy",
    "Wait",
    };

#define SPECIAL_COLOR     math::Vec4( 0.38f, 0.17f, 0.12f, 1.0f )
#define CONDITIONAL_COLOR math::Vec4( 0.14f, 0.41f, 0.17f, 1.0f )
#define STATE_COLOR       math::Vec4( 0.15f, 0.29f, 0.28f, 1.0f )
 
const std::array<math::Vec4, (size_t)StateScriptNodeName::CNT> StateScriptFactory::NodeDisplayColors =
    {
    math::Vec4( 0.0f,  0.0f,  0.0f,  0.0f ),
    SPECIAL_COLOR,
    CONDITIONAL_COLOR,
    STATE_COLOR
    };

const std::array<std::string, (size_t)StateScriptPlugName::CNT> StateScriptFactory::NodePlugDisplayNames =
    {
    "Invalid",
    "In",
    "Begin",
    "Out",
    "OnAbort",
    "OnComplete",
    "SubGraph"
    };

const std::array<std::string, (size_t)StateScriptNodeFilterId::CNT> StateScriptFactory::NodeFilterDisplayNames =
    {
    "Special",
    "Conditional",
    "State"
    };

void assets::StateScriptFactory::SaveProgram( const StateScriptProgram& program, const std::string filename )
{
}

bool assets::StateScriptFactory::LoadProgram( const std::string filename, StateScriptProgram& program )
{
    return false;
}

StateScriptNodeRecord & assets::StateScriptFactory::GetNodeById( const StateScriptNodeId id, const StateScriptProgram &program )
{
return const_cast<StateScriptNodeRecord&>( *std::find_if( program.nodes.begin(), program.nodes.end(), [id]( const StateScriptNodeRecord &record ) { return record.node_id == id; } ) );
}

StateScriptPlugRecord& assets::StateScriptFactory::GetPlugById( const StateScriptPlugId id, const StateScriptProgram &program )
{
return const_cast<StateScriptPlugRecord&>( *std::find_if( program.plugs.begin(), program.plugs.end(), [id]( const StateScriptPlugRecord& record ) { return record.plug_id == id; } ) );
}

StateScriptPlugWireRecord &assets::StateScriptFactory::GetWireById( const StateScriptWireId id, const StateScriptProgram &program )
{
return const_cast<StateScriptPlugWireRecord&>( *std::find_if( program.connections.begin(), program.connections.end(), [id]( const StateScriptPlugWireRecord & record ) { return record.wire_id == id; } ) );
}

StateScriptWireId assets::StateScriptFactory::AddConnectionToProgram( const StateScriptPlugId from, const StateScriptPlugId to, StateScriptProgram &program )
{
assert( IsInputPlug( GetPlugById( from, program ).name ) != IsInputPlug( GetPlugById( to, program ).name ) );
program.connections.emplace_back();
auto &connect = program.connections.back();

connect.wire_id = GenerateNewUid();
connect.from = from;
connect.to = to;

return connect.wire_id;
}

void assets::StateScriptFactory::EnumerateNodePlugs( const StateScriptNodeId node, const StateScriptProgram &program, std::vector<StateScriptPlugId> &out )
{
out.clear();
for( auto &plug : program.plugs )
    {
    if( plug.owner_node_id == node ) out.push_back( plug.plug_id );
    }
}

void assets::StateScriptFactory::EnumeratePlugConnections( const StateScriptPlugId plug, const StateScriptProgram &program, std::vector<StateScriptWireId> &out )
{
out.clear();
for( auto &wire : program.connections )
    {
    if( wire.from == plug
     || wire.to == plug )
        {
        out.push_back( wire.wire_id );
        }
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
    case StateScriptNodeName::ENTRY_POINT:
        AddPlugToNode( node.node_id, StateScriptPlugName::OUT_DEFAULT, program );
        break;

    case StateScriptNodeName::WAIT_FOR_DURATION:
        AddPlugToNode( node.node_id, StateScriptPlugName::IN_BEGIN_STATE, program );
        AddPlugToNode( node.node_id, StateScriptPlugName::OUT_ON_ABORT, program );
        AddPlugToNode( node.node_id, StateScriptPlugName::OUT_ON_COMPLETE, program );
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
