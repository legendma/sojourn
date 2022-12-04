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
            math::Pos2              jack_position = { 0.0f, 0.0f };
            float                   jack_radius = {};
            math::Pos2              label_position = { 0.0f, 0.0f };
            std::string             label = {};
            };

        using Plugs = std::vector<Plug>;
        using PlugsIterator = Plugs::iterator;

        assets::StateScriptNodeId   asset = 0;
        math::ABB2                  abb = { { 0.0f, 0.0f }, { 0.0f, 0.0f } };
        math::Vec4                  color = { 0.0f, 0.0f, 0.0f, 0.0f };
        std::string                 name = {};
        math::Pos2                  name_position;
        Plugs                       in_plugs;
        Plugs                       out_plugs;
        };

    using StateScriptProgramId = uint32_t;

    using StateScriptProgramIds = std::vector<StateScriptProgramId>;
    using StateScriptProgramIdsIterator = StateScriptProgramIds::iterator;

    using StateScriptRenderNodes = std::vector<StateScriptNodeRenderable>;
    using StateScriptRenderNodesIterator = StateScriptRenderNodes::iterator;

    struct StateScriptProgramRecord
        {
        StateScriptProgramId        id = 0;
        std::string                 name = {};
        bool                        is_dirty = false;
        assets::StateScriptProgram  asset;
        StateScriptRenderNodes      nodes;
        std::deque<assets::StateScriptNodeId>
                                    node_draw_order; /* front() == back-most */
        assets::StateScriptNodeId   selected_node = assets::INVALID_ASSET_ID;
        math::Vec2                  view_scroll = { 0, 0 };
        math::Vec2                  last_drag_delta = { 0, 0 };

        struct NewNodeSelectionState
            {
            int                     filter_select_idx = (int)assets::StateScriptNodeFilterId::FIRST_NON_SPECIAL;
            int                     node_id_select_idx = 0;
            } new_node_selection;

        struct CreatingNewNodeConnectionsState
            {
            bool                    is_active = false;
            assets::StateScriptPlugId
                                    originating_plug = assets::INVALID_ASSET_ID;
            } create_node_connections;
        };
    using StateScriptProgramMap = std::unordered_map< StateScriptProgramId, StateScriptProgramRecord >;

    boolean                         is_init = false;
    WorkflowId                      current_workflow = WorkflowId::WORKFLOW_ID_STATESCRIPT;
    StateScriptProgramId            next_statescript_program_id = 1;
    StateScriptProgramId            selected_program_id = 0;
    StateScriptProgramMap           open_programs;

    boolean                         show_imgui_demo = false;

    void ShowMainMenuStateScript();
    void ShowStateScriptPrograms();
    void AddNewStateScriptProgram();
    void BuildNodeRenderable( StateScriptNodeRenderable &node, assets::StateScriptNodeId asset, std::string &name, math::Vec2 &position, math::Vec4 &color, const assets::StateScriptProgram &program );
    void AddNewNodeToProgram( const assets::StateScriptNodeName the_type, StateScriptProgramRecord &program );
    void CloseProgram( StateScriptProgramId program_id, StateScriptProgramIds &pending );
    bool AskUserToSaveProgram( StateScriptProgramId program_id );
    void SaveProgram( StateScriptProgramId program_id );
    void DrawStateScriptNode( const math::Vec2 origin, const bool selected, const StateScriptNodeRenderable &node, const StateScriptProgramRecord &program );
    void BringNodeToFront( const assets::StateScriptNodeId node, StateScriptProgramRecord &program );
    void SendNodeToBack( const assets::StateScriptNodeId node, StateScriptProgramRecord& program );
    bool CollidePointWithTopMostNode( const math::Vec2 view_click, const StateScriptProgramRecord &program, StateScriptRenderNodesIterator& out_node );
    bool CollidePointWithTopMostNodePlug( const math::Vec2 view_click, const StateScriptProgramRecord &program, StateScriptNodeRenderable::PlugsIterator &out_plug, StateScriptRenderNodesIterator &out_node );

public:
    bool ShowWindow();
};