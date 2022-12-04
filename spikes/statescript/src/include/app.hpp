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
        struct Plug
        {
            assets::StateScriptPlugId
                        asset = 0;
            math::Pos2  jack_position = { 0.0f, 0.0f };
            float       jack_radius = {};
            math::Pos2  label_position = { 0.0f, 0.0f };
            std::string label = {};
        };

        assets::StateScriptNodeId
                        asset = 0;
        math::ABB2      abb = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
        math::Vec4      color = { 0.0f, 0.0f, 0.0f, 0.0f };
        std::string     name = {};
        math::Pos2      name_position;
        std::vector<Plug>
                        in_plugs;
        std::vector<Plug>
                        out_plugs;
    };

    using StateScriptProgramId = uint32_t;
    using StateScriptProgramIds = std::vector<StateScriptProgramId>;
    using StateScriptRenderNodes = std::vector<StateScriptNodeRenderable>;

    struct StateScriptProgramRecord
        {
        StateScriptProgramId        id = 0;
        std::string                 name = {};
        bool                        is_dirty = false;
        assets::StateScriptProgram  asset;
        StateScriptRenderNodes      nodes;
        std::deque<assets::StateScriptNodeId>
                                    node_draw_order; /* front() == back-most */
        assets::StateScriptNodeId   selected_node = -1;
        math::Vec2                  view_scroll = { 0, 0 };
        math::Vec2                  last_drag_delta = { 0, 0 };

        struct NewNodeSelection 
            {
            int         filter_select_idx = 0;
            int         node_id_select_idx = 0;
            } new_node_selection;
        };
    using StateScriptProgramMap = std::unordered_map< StateScriptProgramId, StateScriptProgramRecord >;

    WorkflowId                      current_workflow = WorkflowId::WORKFLOW_ID_STATESCRIPT;
    StateScriptProgramId            next_statescript_program_id = 1;
    StateScriptProgramId            selected_program_id = 0;
    StateScriptProgramMap           open_programs;

    void ShowMainMenuStateScript();
    void ShowStateScriptPrograms();
    void AddNewStateScriptProgram();
    void BuildNodeRenderable( StateScriptNodeRenderable &node, assets::StateScriptNodeId asset, std::string &name, math::Vec2 &position, math::Vec4 &color, const assets::StateScriptProgram &program );
    void AddNewNodeToProgram( const assets::StateScriptNodeName the_type, StateScriptProgramRecord &program );
    void CloseProgram( StateScriptProgramId program_id, StateScriptProgramIds &pending );
    bool AskUserToSaveProgram( StateScriptProgramId program_id );
    void SaveProgram( StateScriptProgramId program_id );
    void DrawStateScriptNode( const math::Vec2 origin, const bool selected, const StateScriptNodeRenderable &node, const assets::StateScriptProgram& program );
    void BringNodeToFront( const assets::StateScriptNodeId node, StateScriptProgramRecord &program );
    void SendNodeToBack( const assets::StateScriptNodeId node, StateScriptProgramRecord& program );
    bool GetClickedNode( const math::Vec2 view_click, const StateScriptProgramRecord &program, StateScriptRenderNodes::iterator& out_node );

public:
    bool ShowWindow();
};