#pragma once

namespace assets
{
using StateScriptUID = uint32_t;
using StateScriptNodeId = StateScriptUID;
using StateScriptPlugId = StateScriptUID;
using StateScriptWireId = StateScriptUID;

static const StateScriptUID INVALID_ASSET_ID = std::numeric_limits<StateScriptUID>::max();

enum class StateScriptNodeName
{
    INVALID,
    ENTRY_POINT,

    /* Conditional */
    CONDITIONAL_FIRST,
    DUMMY_DELETE_ME  = CONDITIONAL_FIRST,
    CONDITIONAL_LAST = DUMMY_DELETE_ME,

    /* States */
    STATE_FIRST,
    WAIT_FOR_DURATION = STATE_FIRST,
    STATE_LAST        = WAIT_FOR_DURATION,

    /* Count */
    CNT
};

enum class StateScriptNodeFilterId
    {
    SPECIAL,
    FIRST_NON_SPECIAL,

    CONDITIONAL = FIRST_NON_SPECIAL,
    STATE,
    /* Count */
    CNT
    };

static const std::array<StateScriptNodeName,1> SPECIAL_NODES_FILTER =
    {
    StateScriptNodeName::ENTRY_POINT
    };

static const std::array<StateScriptNodeName,1> CONDITIONAL_NODES_FILTER =
    {
    StateScriptNodeName::DUMMY_DELETE_ME
    };

static const std::array<StateScriptNodeName,1> STATE_NODES_FILTER =
    {
    StateScriptNodeName::WAIT_FOR_DURATION
    };

struct StateScriptNodeRecord
{
    StateScriptNodeId   node_id;
    StateScriptNodeName name;
};

enum class StateScriptPlugName
{
    INVALID,

    /* In plugs */
    FIRST_IN,
    IN_DEFAULT = FIRST_IN,
    IN_BEGIN_STATE,
    LAST_IN    = IN_BEGIN_STATE,

    /* Out plugs */
    FIRST_OUT,
    OUT_DEFAULT = FIRST_OUT,
    OUT_ON_ABORT,
    OUT_ON_COMPLETE,
    OUT_SUBGRAPH,
    LAST_OUT    = OUT_SUBGRAPH,

    /* Count */
    CNT
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
    static StateScriptWireId AddConnectionToProgram( const StateScriptPlugId from, const StateScriptPlugId to, StateScriptProgram &program );
    static StateScriptNodeRecord & GetNodeById( const StateScriptNodeId id, const StateScriptProgram &program );
    static StateScriptPlugRecord & GetPlugById( const StateScriptPlugId id, const StateScriptProgram &program );
    static StateScriptPlugWireRecord & GetWireById( const StateScriptWireId id, const StateScriptProgram &program );
    static void EnumerateNodePlugs( const StateScriptNodeId node, const StateScriptProgram &program, std::vector<StateScriptPlugId> &out );
    static void EnumeratePlugConnections( const StateScriptPlugId plug, const StateScriptProgram &program, std::vector<StateScriptWireId> &out );
    
    static inline bool IsInputPlug( const StateScriptPlugName plug ) { return( plug >= StateScriptPlugName::FIRST_IN
                                                                            && plug <= StateScriptPlugName::LAST_IN ); }
    
    static inline bool IsOutputPlug( const StateScriptPlugName plug ) { return( plug >= StateScriptPlugName::FIRST_OUT
                                                                             && plug <= StateScriptPlugName::LAST_OUT ); }

    static const std::array<std::string, (size_t)StateScriptNodeName::CNT> NodeDisplayNames;
    static const std::array<math::Vec4, (size_t)StateScriptNodeName::CNT> NodeDisplayColors;

    static const std::array<std::string, (size_t)StateScriptPlugName::CNT> NodePlugDisplayNames;

    static const std::array<std::string, (size_t)StateScriptNodeFilterId::CNT> NodeFilterDisplayNames;

private:
    static StateScriptPlugId AddPlugToNode( const StateScriptNodeId node, const StateScriptPlugName the_type, StateScriptProgram& program );

    static uint32_t GenerateNewUid();
};

}; // namespace assets