#pragma once

namespace assets
{

using StateScriptNodeId = uint32_t;
using StateScriptPlugId = uint32_t;
using StateScriptWireId = uint32_t;

enum class StateScriptNodeName
{
    STATESCRIPT_NODE_NAME_INVALID,
    STATESCRIPT_NODE_NAME_ENTRY_POINT,
    STATESCRIPT_NODE_NAME_CNT
};

struct StateScriptNodeRecord
{
    StateScriptNodeId   node_id;
    StateScriptNodeName name;
};

enum StateScriptPlugName
{
    STATESCRIPT_PLUG_NAME_INVALID,
    /* In plugs */
    STATESCRIPT_PLUG_NAME_IN_FIRST,
    STATESCRIPT_PLUG_NAME_IN_DEFAULT = STATESCRIPT_PLUG_NAME_IN_FIRST,
    STATESCRIPT_PLUG_NAME_IN_ACTIVATE,
    STATESCRIPT_PLUG_NAME_IN_LAST = STATESCRIPT_PLUG_NAME_IN_ACTIVATE,
    /* Out plugs */
    STATESCRIPT_PLUG_NAME_OUT_FIRST,
    STATESCRIPT_PLUG_NAME_OUT_DEFAULT = STATESCRIPT_PLUG_NAME_OUT_FIRST,
    STATESCRIPT_PLUG_NAME_OUT_LAST = STATESCRIPT_PLUG_NAME_OUT_DEFAULT,
    /* Count */
    STATESCRIPT_PLUG_NAME_CNT
} ;

struct StateScriptPlugRecord
{
    StateScriptPlugId   plug_id;
    StateScriptPlugName name;
    StateScriptNodeId   owner_node_id;
};

struct StateScriptPlugWireRecord
{
    StateScriptWireId   wire_id;
    StateScriptPlugId   from;
    StateScriptPlugId   to;
};

struct StateScriptProgram
{
    using StateScriptNodeTable = std::vector<StateScriptNodeRecord>;
    using StateScriptPlugTable = std::vector<StateScriptPlugRecord>;
    using StateScriptPlugConnectionTable = std::vector<StateScriptPlugWireRecord>;

    StateScriptNodeTable
                        nodes = {};
    StateScriptPlugTable
                        plugs = {};
    StateScriptPlugConnectionTable
                        connections = {};
};

class StateScriptFactory
{
public:
    static void SaveProgram( const StateScriptProgram &program, const std::string filename );
    static bool LoadProgram( const std::string filename, StateScriptProgram &program );

    static StateScriptNodeId AddNodeToProgram( const StateScriptNodeName the_type, StateScriptProgram& program );
    StateScriptNodeRecord & GetNodeById( const StateScriptNodeId id, StateScriptProgram& program );
    static StateScriptPlugRecord & GetPlugById( const StateScriptPlugId id, StateScriptProgram &program );
    static void GetPlugsForNode( const StateScriptNodeId node, const StateScriptProgram &program, std::vector<StateScriptPlugId> &out );

    static const std::array<std::string, (size_t)StateScriptNodeName::STATESCRIPT_NODE_NAME_CNT> NodeDisplayNames;
    static const std::array<Vec4, (size_t)StateScriptNodeName::STATESCRIPT_NODE_NAME_CNT> NodeDisplayColors;

    static const std::array<std::string, (size_t)StateScriptPlugName::STATESCRIPT_PLUG_NAME_CNT> NodePlugDisplayNames;

private:
    static StateScriptPlugId AddPlugToNode( const StateScriptNodeId node, const StateScriptPlugName the_type, StateScriptProgram& program );

    static uint32_t GenerateNewUid();
};

}; // namespace assets