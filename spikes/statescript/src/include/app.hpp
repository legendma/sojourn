#pragma once

class App
{
    enum class WorkflowId
        {
        WORKFLOW_ID_STATESCRIPT
        };

    using StateScriptProgramId = uint32_t;
    using StateScriptProgramIds = std::vector<StateScriptProgramId>;

    struct StateScriptProgramRecord
        {
        StateScriptProgramId        id = 0;
        std::string                 name = {};
        bool                        is_dirty = false;
        };
    using StateScriptProgramMap = std::unordered_map< StateScriptProgramId, StateScriptProgramRecord >;

    WorkflowId                      current_workflow = WorkflowId::WORKFLOW_ID_STATESCRIPT;
    StateScriptProgramId            next_statescript_program_id = 1;
    StateScriptProgramId            selected_program_id = 0;
    StateScriptProgramMap           open_programs;

    void ShowMainMenuStateScript();
    void ShowStateScriptPrograms();
    void AddNewStateScriptProgram();
    void AddNewModuleToSelectedProgram();
    void CloseProgram( StateScriptProgramId program_id, StateScriptProgramIds &pending );
    bool AskUserToSaveProgram( StateScriptProgramId program_id );
    void SaveProgram( StateScriptProgramId program_id );

public:
    bool ShowWindow();
};