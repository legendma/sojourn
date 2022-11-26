#pragma once

#include "statescript.hpp"

class App
{
    enum class WorkflowId
        {
        WORKFLOW_ID_STATESCRIPT
        };

    struct StateScriptNodeRenderable
    {
        assets::StateScriptNodeId
                        asset;
        Pos2            position;
        Vec2            extent;
        Vec4            color;
        std::string     display_name;
    };

    using StateScriptProgramId = uint32_t;
    using StateScriptProgramIds = std::vector<StateScriptProgramId>;

    struct StateScriptProgramRecord
        {
        StateScriptProgramId        id = 0;
        std::string                 name = {};
        bool                        is_dirty = false;
        assets::StateScriptProgram  asset;
        std::vector<StateScriptNodeRenderable>
                                    nodes;
        };
    using StateScriptProgramMap = std::unordered_map< StateScriptProgramId, StateScriptProgramRecord >;

    WorkflowId                      current_workflow = WorkflowId::WORKFLOW_ID_STATESCRIPT;
    StateScriptProgramId            next_statescript_program_id = 1;
    StateScriptProgramId            selected_program_id = 0;
    StateScriptProgramMap           open_programs;

    void ShowMainMenuStateScript();
    void ShowStateScriptPrograms();
    void AddNewStateScriptProgram();
    void AddNewNodeToProgram( const assets::StateScriptNodeName the_type, StateScriptProgramRecord &program );
    void CloseProgram( StateScriptProgramId program_id, StateScriptProgramIds &pending );
    bool AskUserToSaveProgram( StateScriptProgramId program_id );
    void SaveProgram( StateScriptProgramId program_id );
    void DrawStateScriptNode( const StateScriptNodeRenderable &node );

public:
    bool ShowWindow();
};