/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "../StandardPluginsConfig.h"
#include "BlendGraphWidget.h"
#include "BlendTreeVisualNode.h"
#include "BlendGraphViewWidget.h"
#include "StateGraphNode.h"
#include "NodeGraph.h"
#include "AttributesWindow.h"
#include "AnimGraphPlugin.h"
#include "NavigateWidget.h"
#include "NodePaletteWidget.h"
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MetricsEventSender.h>

// qt includes
#include <QDropEvent>
#include <QMenu>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QToolTip>
#include <QTreeWidget>
#include <QPushButton>
#include <QFont>
#include <QMimeData>

// emfx and core includes
#include <MCore/Source/LogManager.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphExitNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/MotionManager.h>

// emstudio SDK
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <MysticQt/Source/LinkWidget.h>
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>

#include <AzQtComponents/Utilities/Conversions.h>

namespace EMStudio
{
    class BlendGraphRenderNodeGroupsCallback
        : public NodeGraph::NodeGroupsRenderCallback
    {
    public:
        BlendGraphRenderNodeGroupsCallback(AnimGraphPlugin* plugin, BlendGraphWidget* widget)
        {
            mPlugin = plugin;
            mWidget = widget;
            mFont.setPixelSize(18);
            mTempString.reserve(64);

            mFontMetrics = new QFontMetrics(mFont);
        }

        ~BlendGraphRenderNodeGroupsCallback()
        {
            delete mFontMetrics;
        }

        void RenderNodeGroups(QPainter& painter, NodeGraph* nodeGraph)
        {
            EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();

            // check if all objects are valid that we need
            if (animGraph == nullptr || nodeGraph == nullptr)
            {
                return;
            }

            // get the number of node groups and iterate through them
            QRect nodeRect;
            QRect groupRect;
            const uint32 numNodeGroups = animGraph->GetNumNodeGroups();
            for (uint32 i = 0; i < numNodeGroups; ++i)
            {
                // get the current node group
                EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->GetNodeGroup(i);

                // skip the node group if it isn't visible
                if (nodeGroup->GetIsVisible() == false)
                {
                    continue;
                }

                // get the number of nodes inside the node group and skip the group in case there are no nodes in
                const uint32 numNodes = nodeGroup->GetNumNodes();
                if (numNodes == 0)
                {
                    continue;
                }

                int32 top   =  std::numeric_limits<int32>::max();
                int32 bottom =  std::numeric_limits<int32>::lowest();
                int32 left  =  std::numeric_limits<int32>::max();
                int32 right =  std::numeric_limits<int32>::lowest();

                bool nodesInGroupDisplayed = false;
                for (uint32 j = 0; j < numNodes; ++j)
                {
                    // get the graph node by the id and skip it if the node is not inside the currently visible node graph
                    const EMotionFX::AnimGraphNodeId nodeId  = nodeGroup->GetNode(j);
                    GraphNode* graphNode = nodeGraph->FindNodeById(nodeId);
                    if (graphNode == nullptr)
                    {
                        continue;
                    }

                    nodesInGroupDisplayed = true;

                    nodeRect    = graphNode->GetRect();
                    top         = MCore::Min3<int32>(top,      nodeRect.top(), nodeRect.bottom());
                    bottom      = MCore::Max3<int32>(bottom,   nodeRect.top(), nodeRect.bottom());
                    left        = MCore::Min3<int32>(left, nodeRect.left(), nodeRect.right());
                    right       = MCore::Max3<int32>(right,    nodeRect.left(), nodeRect.right());
                }

                if (nodesInGroupDisplayed)
                {
                    // get the color from the node group and set it to the painter
                    AZ::Color azColor;
                    azColor.FromU32(nodeGroup->GetColor());
                    QColor color = AzQtComponents::ToQColor(azColor);
                    color.setAlpha(150);
                    painter.setPen(color);
                    color.setAlpha(40);
                    painter.setBrush(color);

                    const int32 border = 10;
                    groupRect.setTop(top - (border + 15));
                    groupRect.setBottom(bottom + border);
                    groupRect.setLeft(left - border);
                    groupRect.setRight(right + border);
                    painter.drawRoundedRect(groupRect, 7, 7);

                    QRect textRect = groupRect;
                    textRect.setHeight(mFontMetrics->height());
                    textRect.setLeft(textRect.left() + border);

                    // draw the name on top
                    color.setAlpha(255);
                    //painter.setPen( color );
                    //mTempString = nodeGroup->GetName();
                    //painter.setFont( mFont );
                    GraphNode::RenderText(painter, nodeGroup->GetName(), color, mFont, *mFontMetrics, Qt::AlignLeft, textRect);
                    //painter.drawText( left - 7, top - 7, mTempString );
                }
            }   // for all node groups
        }

    private:
        AnimGraphPlugin*   mPlugin;
        BlendGraphWidget*   mWidget;
        QString             mTempString;
        QFont               mFont;
        QFontMetrics*       mFontMetrics;
    };


    // constructor
    BlendGraphWidget::BlendGraphWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : NodeGraphWidget(plugin, nullptr, parent)
    {
        mCurrentNode                = nullptr;
        //mMouseClickTimer          = new QTimer(this);
        //connect( mMouseClickTimer, SIGNAL( timeout() ), SLOT( OnMouseClickTimeout() ) );

        mOverlayFont.setPixelSize(10);
        mMoveGroup.SetGroupName("Move anim graph nodes");

        setAutoFillBackground(false);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }


    // destructor
    BlendGraphWidget::~BlendGraphWidget()
    {
    }


    void BlendGraphWidget::SetCurrentNode(EMotionFX::AnimGraphNode* node)
    {
        mCurrentNode = node;
    }


    void BlendGraphWidget::OnDrawOverlay(QPainter& painter)
    {
        MCORE_UNUSED(painter);

        /*  if (mActiveGraph == nullptr)
                return;

            if (mCurrentNode)
                mTextString = AZStd::string::format("%s (%d nodes)", mCurrentNode->GetName(), mActiveGraph->GetNumNodes());
            else
                mTextString = AZStd::string::format("ROOT (%d state machines)", mActiveGraph->GetNumNodes());

            painter.setPen( QColor(255,128,0) );
            painter.resetTransform();
            painter.setFont( mOverlayFont );
            painter.drawText(10, 40, mTextString.AsChar());*/
    }


    // when dropping stuff in our window
    void BlendGraphWidget::dropEvent(QDropEvent* event)
    {
        // dont accept dragging/drop from and to yourself
        if (event->source() == this)
        {
            return;
        }

        // only accept copy actions
        if (event->dropAction() != Qt::CopyAction || event->mimeData()->hasText() == false)
        {
            event->ignore();
            return;
        }

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // if we have text, get it
        AZStd::string dropText = FromQtString(event->mimeData()->text());
        MCore::CommandLine commandLine(dropText.c_str());

        // calculate the drop position
        QPoint offset = LocalToGlobal(event->pos());

        // check if the drag & drop is coming from an external window
        if (commandLine.CheckIfHasParameter("window"))
        {
            AZStd::string currentLine;
            
            AZStd::vector<AZStd::string> droppedLines;
            AzFramework::StringFunc::Tokenize(dropText.c_str(), droppedLines, "\n", false, true);

            const size_t numDroppedLines = droppedLines.size();
            for (size_t l = 0; l < numDroppedLines; ++l)
            {
                MCore::CommandLine currentCommandLine(droppedLines[l].c_str());

                // get the name of the window where the drag came from
                AZStd::string dragWindow;
                currentCommandLine.GetValue("window", "", dragWindow);

                // drag&drop coming from the motion set window from the standard plugins
                if (dragWindow == "MotionSetWindow")
                {
                    AZStd::string motionId;
                    currentCommandLine.GetValue("motionNameID", "", motionId);

                    EMotionFX::AnimGraphMotionNode tempMotionNode;
                    AZStd::vector<AZStd::string> motionIds;
                    motionIds.emplace_back(motionId);
                    tempMotionNode.SetMotionIds(motionIds);

                    AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::Serialize(&tempMotionNode);
                    if (serializedMotionNode.IsSuccess())
                    {
                        CommandSystem::CreateAnimGraphNode(animGraph, "BlendTreeMotionNode", "Motion", mCurrentNode, offset.x(), offset.y(), serializedMotionNode.GetValue());

                        // Send LyMetrics event.
                        MetricsEventSender::SendCreateNodeEvent(azrtti_typeid<EMotionFX::AnimGraphMotionNode>());

                        // setup the offset for the next motion
                        offset.setY(offset.y() + 60);
                    }
                }

                // drag&drop coming from the motion window from the standard plugins
                if (dragWindow == "MotionWindow")
                {
                    // get the motion id and the corresponding motion object
                    uint32 motionID = currentCommandLine.GetValueAsInt("motionID", MCORE_INVALIDINDEX32);
                    EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);

                    // get the selected actor instance
                    CommandSystem::SelectionList& selectionList = EMStudio::GetCommandManager()->GetCurrentSelection();
                    EMotionFX::ActorInstance* actorInstance = selectionList.GetSingleActorInstance();

                    if (actorInstance == nullptr || motion == nullptr)
                    {
                        QMessageBox::warning(this, "Cannot Complete Drop Operation", "Please select a single actor instance before dropping the motion.");
                        event->ignore();
                        return;
                    }

                    // get the anim graph instance from the current actor instance and check if it is valid
                    EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                    if (animGraphInstance == nullptr)
                    {
                        QMessageBox::warning(this, "Cannot Complete Drop Operation", "Please activate a anim graph before dropping the motion.");
                        event->ignore();
                        return;
                    }

                    // get the motion set from the anim graph instance and check if it is valid
                    EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
                    if (motionSet == nullptr)
                    {
                        QMessageBox::warning(this, "No Active Motion Set", "Cannot drop the motion to the anim graph. Please assign a motion set to the anim graph before.");
                        event->ignore();
                        return;
                    }

                    // try to find the motion entry for the given motion
                    EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntry(motion);
                    if (motionEntry)
                    {
                        EMotionFX::AnimGraphMotionNode tempMotionNode;
                        AZStd::vector<AZStd::string> motionIds;
                        motionIds.emplace_back(motionEntry->GetId());
                        tempMotionNode.SetMotionIds(motionIds);

                        AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::Serialize(&tempMotionNode);
                        if (serializedMotionNode.IsSuccess())
                        {
                            CommandSystem::CreateAnimGraphNode(animGraph, azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", mCurrentNode, offset.x(), offset.y(), serializedMotionNode.GetValue());

                            // Send LyMetrics event.
                            MetricsEventSender::SendCreateNodeEvent(azrtti_typeid<EMotionFX::AnimGraphMotionNode>());

                            // setup the offset for the next motion
                            offset.setY(offset.y() + 60);
                        }
                    }
                    else
                    {
                        if (QMessageBox::warning(this, "Motion Not Part Of Motion Set", "Do you want the motion to be automatically added to the active motion set? When pressing no the drop action will be canceled.", QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                        {
                            event->ignore();
                            return;
                        }

                        // Build a list of unique string id values from all motion set entries.
                        AZStd::vector<AZStd::string> idStrings;
                        motionSet->BuildIdStringList(idStrings);

                        // remove the media root folder from the absolute motion filename so that we get the relative one to the media root folder
                        AZStd::string motionEntryFileName = motion->GetFileName();
                        EMotionFX::GetEMotionFX().GetFilenameRelativeToMediaRoot(&motionEntryFileName);

                        if (EMotionFX::MotionSet::MotionEntry::CheckIfIsAbsoluteFilename(motionEntryFileName.c_str()))
                        {
                            AZStd::string text = AZStd::string::format("Some of the motions are located outside of the asset folder of your project:\n\n%s\n\nThis means that the motion set cannot store relative filenames and will hold absolute filenames.", EMotionFX::GetEMotionFX().GetMediaRootFolder());
                            QMessageBox::warning(this, "Warning", text.c_str());
                        }

                        const AZStd::string idString = CommandSystem::AddMotionSetEntry(motionSet->GetID(), "", idStrings, motionEntryFileName.c_str());

                        EMotionFX::AnimGraphMotionNode tempMotionNode;
                        AZStd::vector<AZStd::string> motionIds;
                        motionIds.emplace_back(idString);
                        tempMotionNode.SetMotionIds(motionIds);

                        AZ::Outcome<AZStd::string> serializedMotionNode = MCore::ReflectionSerializer::Serialize(&tempMotionNode);
                        if (serializedMotionNode.IsSuccess())
                        {
                            CommandSystem::CreateAnimGraphNode(animGraph, azrtti_typeid<EMotionFX::AnimGraphMotionNode>(), "Motion", mCurrentNode, offset.x(), offset.y(), serializedMotionNode.GetValue());

                            // Send LyMetrics event.
                            MetricsEventSender::SendCreateNodeEvent(azrtti_typeid<EMotionFX::AnimGraphMotionNode>());

                            // setup the offset for the next motion
                            offset.setY(offset.y() + 60);
                        }
                    }
                }
            }
        }
        // default handling, we're drag & dropping from the palette window
        else
        {
            AZStd::vector<AZStd::string> parts;
            AzFramework::StringFunc::Tokenize(dropText.c_str(), parts, ";", false, true);
            if (parts.size() != 3)
            {
                MCore::LogError("BlendGraphWidget::dropEvent() - Incorrect syntax using drop data '%s'", FromQtString(event->mimeData()->text()).c_str());
                event->ignore();
                return;
            }

            AZStd::string commandString;
            AZStd::string resultString;
            if (mCurrentNode == nullptr)
            {
                if (AzFramework::StringFunc::Equal(parts[1].c_str(), azrtti_typeid<EMotionFX::AnimGraphStateMachine>().ToString<AZStd::string>().c_str(), false /* no case */) == false)
                {
                    MCore::LogError("You can only drop State Machines as root nodes!");
                    event->ignore();
                    return;
                }
            }

            // build the name prefix
            AZStd::string& namePrefix = parts[2];
            AzFramework::StringFunc::Strip(namePrefix, MCore::CharacterConstants::space, true /* case sensitive */);

            const AZ::TypeId typeId = AZ::TypeId::CreateString(parts[1].c_str(), parts[1].size());
            CommandSystem::CreateAnimGraphNode(animGraph, typeId, namePrefix, mCurrentNode, offset.x(), offset.y());

            // Send LyMetrics event.
            MetricsEventSender::SendCreateNodeEvent(typeId);
        }

        event->accept();
    }


    bool BlendGraphWidget::OnEnterDropEvent(QDragEnterEvent* event, EMotionFX::AnimGraphNode* currentNode, NodeGraph* activeGraph)
    {
        if (event->mimeData()->hasText() == false)
        {
            return false;
        }

        // if we have text, get it
        AZStd::string dropText = FromQtString(event->mimeData()->text());
        MCore::CommandLine commandLine(dropText.c_str());

        // check if the drag & drop is coming from an external window
        if (commandLine.CheckIfHasParameter("window"))
        {
            // in case the current node is nullptr and the active graph is a valid graph it means we are showing the root graph
            if (currentNode == nullptr)
            {
                QMessageBox::warning(GetMainWindow(), "Cannot Drop Motion", "Either there is no node shown or you are trying to add a motion to the root level which is not possible.");
                return false;
            }

            // check if we need to prevent dropping of non-state machine nodes
            if (azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>() ||
                azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::BlendTree>())
            {
                return true;
            }

            return false;
        }

        AZStd::vector<AZStd::string> parts;
        AzFramework::StringFunc::Tokenize(dropText.c_str(), parts, MCore::CharacterConstants::semiColon, true /* keep empty strings */, true /* keep space strings */);

        if (parts.size() != 3)
        {
            //MCore::LogError("BlendGraphWidget::dropEvent() - Incorrect syntax using drop data '%s'", FromQtString(event->mimeData()->text()).AsChar());
            return false;
        }

        if (parts[0].find("EMotionFX::AnimGraphNode") != 0)
        {
            return false;
        }

        // check if the dropped node is a state machine node
        bool canActAsState = false;
        const AZ::TypeId typeId = AZ::TypeId::CreateString(parts[1].c_str(), parts[1].size());
        if (!typeId.IsNull())
        {
            EMotionFX::AnimGraphObject* registeredObject = EMotionFX::AnimGraphObjectFactory::Create(typeId);
            MCORE_ASSERT(azrtti_istypeof<EMotionFX::AnimGraphNode>(registeredObject));

            EMotionFX::AnimGraphNode* registeredNode = static_cast<EMotionFX::AnimGraphNode*>(registeredObject);
            if (registeredNode->GetCanActAsState())
            {
                canActAsState = true;
            }

            delete registeredObject;
        }

        // in case the current node is nullptr and the active graph is a valid graph it means we are showing the root graph
        if (currentNode == nullptr)
        {
            if (activeGraph)
            {
                // only state machine nodes are allowed in the root graph
                if (canActAsState)
                {
                    return true;
                }
            }
        }
        else
        {
            // check if we need to prevent dropping of non-state machine nodes
            if (azrtti_typeid(currentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                if (canActAsState)
                {
                    return true;
                }
            }
            else
            {
                return true;
            }
        }

        return false;
    }


    // drag enter
    void BlendGraphWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        //MCore::LogDebug("BlendGraphWidget::dragEnter");
        bool acceptEnterEvent = OnEnterDropEvent(event, mCurrentNode, GetActiveGraph());

        if (acceptEnterEvent)
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }


    // drag leave
    void BlendGraphWidget::dragLeaveEvent(QDragLeaveEvent* event)
    {
        //MCore::LogDebug("BlendGraphWidget::dragLeave");
        event->accept();
    }


    // moving around while dragging
    void BlendGraphWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        MCORE_UNUSED(event);
        // MCore::LogDebug("BlendGraphWidget::dragMove");
    }


    void BlendGraphWidget::OnContextMenuCreateNode()
    {
        assert(sender()->inherits("QAction"));
        QAction* action = qobject_cast<QAction*>(sender());

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // calculate the position
        const QPoint offset = SnapLocalToGrid(LocalToGlobal(mContextMenuEventMousePos), 10);

        // build the name prefix and create the node
        const AZStd::string typeString = FromQtString(action->whatsThis());
        AZStd::string namePrefix = FromQtString(action->data().toString());
        AzFramework::StringFunc::Strip(namePrefix, MCore::CharacterConstants::space, true /* case sensitive */);
        const AZ::TypeId typeId = AZ::TypeId::CreateString(typeString.c_str(), typeString.size());
        CommandSystem::CreateAnimGraphNode(animGraph, typeId, namePrefix, mCurrentNode, offset.x(), offset.y());

        // Send LyMetrics event.
        MetricsEventSender::SendCreateNodeEvent(typeId);
    }


    bool BlendGraphWidget::CheckIfIsStateMachine()
    {
        EMotionFX::AnimGraph*  animGraph  = mPlugin->GetActiveAnimGraph();
        NodeGraph*              nodeGraph   = GetActiveGraph();
        if (nodeGraph == nullptr || animGraph == nullptr)
        {
            return false;
        }

        //bool isStateMachine = false;
        const uint32 numNodes = nodeGraph->GetNumNodes();
        if (numNodes > 0)
        {
            GraphNode* node = nodeGraph->GetNode(0);
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(node->GetName());
            if (animGraphNode)
            {
                if (animGraphNode->GetParentNode())
                {
                    // if the parent is a state machine return success
                    if (azrtti_typeid(animGraphNode->GetParentNode()) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }


    void BlendGraphWidget::CollectSelectedConnections(MCore::Array<NodeConnection*>* outConnections)
    {
        // clear the array upfront
        outConnections->Clear();

        EMotionFX::AnimGraph*  animGraph  = mPlugin->GetActiveAnimGraph();
        NodeGraph*              nodeGraph   = GetActiveGraph();
        if (nodeGraph == nullptr || animGraph == nullptr)
        {
            return;
        }

        // get the number of nodes in the current visible graph and iterate through them
        const uint32 numNodes = nodeGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = nodeGraph->GetNode(i);

            // get the number of connections and iterate through them
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = node->GetConnection(c);

                // is the connection selected and not in the array yet? in case both tests succeed add it to the array
                if (connection->GetIsSelected() && outConnections->Find(connection) == MCORE_INVALIDINDEX32)
                {
                    outConnections->Add(connection);
                }
            }
        }
    }


    // enable or disable all selected transitions
    void BlendGraphWidget::SetSelectedTransitionsEnabled(bool isEnabled)
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // only allowed when a state machine is currently being showed
        if (CheckIfIsStateMachine() == false)
        {
            return;
        }

        // gather the selected transitions
        MCore::Array<NodeConnection*> selectedTransitions;
        CollectSelectedConnections(&selectedTransitions);
        const uint32 numTransitions = selectedTransitions.GetLength();

        MCore::CommandGroup commandGroup("Enable/disable transitions", numTransitions);

        // iterate through the selected transitions and and enable or disable them
        AZStd::string commandString;
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // get the transition and its visual representation
            StateConnection*                      visualTransition  = static_cast<StateConnection*>(selectedTransitions[i]);
            EMotionFX::AnimGraphStateTransition* transition        = FindTransitionForConnection(visualTransition);

            // get the target node
            EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
            if (targetNode == nullptr)
            {
                MCore::LogError("Cannot enable/disable transition with id %i. Target node is invalid.", transition->GetID());
                continue;
            }

            // get the parent node of the target node
            EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();
            if (parentNode == nullptr || (parentNode && azrtti_typeid(parentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>()))
            {
                MCore::LogError("Cannot enable/disable transition with id %i. Parent node is invalid.", transition->GetID());
                continue;
            }

            // enable or disable the transition and sync it with the visual version
            commandString = AZStd::string::format("AnimGraphAdjustConnection -animGraphID %i -stateMachine \"%s\" -transitionID %i -isDisabled %s", animGraph->GetID(), parentNode->GetName(), transition->GetID(), AZStd::to_string(!isEnabled).c_str());
            commandGroup.AddCommandString(commandString.c_str());
        }

        // execute the command
        AZStd::string resultString;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }


    void BlendGraphWidget::OnContextMenuEvent(QPoint mousePos, QPoint globalMousePos)
    {
        uint32 i;

        if (!mAllowContextMenu)
        {
            return;
        }

        EMotionFX::AnimGraph*  animGraph  = mPlugin->GetActiveAnimGraph();
        NodeGraph*              nodeGraph   = GetActiveGraph();
        if (!nodeGraph || !animGraph)
        {
            return;
        }

        // Early out in case we're adjusting or creating a new connection. Elsewise the user can open the context menu and
        // delete selected nodes while creating a new connection.
        if (nodeGraph->GetIsCreatingConnection() || nodeGraph->GetIsRelinkingConnection() ||
            nodeGraph->GetRepositionedTransitionHead() || nodeGraph->GetRepositionedTransitionTail()) 
        {
            return;
        }

        mContextMenuEventMousePos = mousePos;

        // get the array of selected anim graph nodes
        MCore::Array<EMotionFX::AnimGraphNode*> selectedNodes;
        MCore::Array<GraphNode*> selectedGraphNodes = nodeGraph->GetSelectedNodes();
        const uint32 numSelectedNodes = selectedGraphNodes.GetLength();
        for (i = 0; i < numSelectedNodes; ++i)
        {
            EMotionFX::AnimGraphNode* animGraphNode = animGraph->RecursiveFindNodeByName(selectedGraphNodes[i]->GetName());
            if (animGraphNode)
            {
                selectedNodes.Add(animGraphNode);
            }
        }

        // get the array of selected connections
        MCore::Array<NodeConnection*> selectedConnections;
        bool mouseOverSelectedConnection = false;
        const uint32 numNodes = nodeGraph->GetNumNodes();
        for (i = 0; i < numNodes; ++i)
        {
            GraphNode* node = nodeGraph->GetNode(i);

            // get the number of connections and iterate through them
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = node->GetConnection(c);

                // is the connection selected?
                if (connection->GetIsSelected())
                {
                    selectedConnections.Add(connection);

                    // is the mouse over the connection?
                    if (connection->CheckIfIsCloseTo(LocalToGlobal(mousePos)))
                    {
                        mouseOverSelectedConnection = true;
                    }
                }
            }
        }

        if (selectedNodes.GetIsEmpty() && selectedConnections.GetIsEmpty() == false && numNodes > 0 && mouseOverSelectedConnection)
        {
            // create the context menu
            QMenu menu(this);

            AZStd::string actionName;
            if (CheckIfIsStateMachine())
            {
                if (selectedConnections.GetLength() == 1)
                {
                    // allow the remove transition action
                    actionName = "Remove Transition";

                    // enable / disable transition
                    NodeConnection* connection = selectedConnections[0];
                    if (connection->GetIsDisabled())
                    {
                        QAction* enableConnectionAction = menu.addAction("Enable Transition");
                        connect(enableConnectionAction, SIGNAL(triggered()), this, SLOT(EnableSelectedTransitions()));
                    }
                    else
                    {
                        QAction* disableConnectionAction = menu.addAction("Disable Transition");
                        connect(disableConnectionAction, SIGNAL(triggered()), this, SLOT(DisableSelectedTransitions()));
                    }


                    EMotionFX::AnimGraphStateTransition* transition = FindTransitionForConnection(connection);
                    if (transition)
                    {
                        // allow to put the conditions into the clipboard
                        if (transition->GetNumConditions() > 0)
                        {
                            QAction* copyAction = menu.addAction("Copy Conditions");
                            copyAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Copy.png"));
                            connect(copyAction, SIGNAL(triggered()), (QWidget*)mPlugin->GetAttributesWindow(), SLOT(OnCopyConditions()));
                        }

                        // if we already copied some conditions, allow pasting
                        if (mPlugin->GetAttributesWindow()->GetCopyPasteConditionClipboard().GetIsEmpty() == false)
                        {
                            QAction* pasteAction = menu.addAction("Paste Conditions");
                            pasteAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
                            connect(pasteAction, SIGNAL(triggered()), (QWidget*)mPlugin->GetAttributesWindow(), SLOT(OnPasteConditions()));

                            QAction* pasteSelectiveAction = menu.addAction("Paste Conditions Selective");
                            pasteSelectiveAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Paste.png"));
                            connect(pasteSelectiveAction, SIGNAL(triggered()), mPlugin->GetAttributesWindow(), SLOT(OnPasteConditionsSelective()));
                        }
                    }
                }
                else
                {
                    actionName = "Remove Transitions";

                    // check if we need to show the disable and enable transitions action
                    bool showDisableTransitions = false;
                    bool showEnableTransitions = false;
                    const uint32 numSelectedConnections = selectedConnections.GetLength();
                    for (i = 0; i < numSelectedConnections; ++i)
                    {
                        NodeConnection* connection = selectedConnections[i];
                        if (connection->GetIsDisabled())
                        {
                            showEnableTransitions = true;
                        }
                        else
                        {
                            showDisableTransitions = true;
                        }
                    }

                    // enable transitions menu entry
                    if (showEnableTransitions)
                    {
                        QAction* enableConnectionAction = menu.addAction("Enable Transitions");
                        connect(enableConnectionAction, SIGNAL(triggered()), this, SLOT(EnableSelectedTransitions()));
                    }

                    // disable transitions menu entry
                    if (showDisableTransitions)
                    {
                        QAction* disableConnectionAction = menu.addAction("Disable Transitions");
                        connect(disableConnectionAction, SIGNAL(triggered()), this, SLOT(DisableSelectedTransitions()));
                    }
                }
            }
            else
            {
                if (selectedConnections.GetLength() == 1)
                {
                    actionName = "Remove Connection";
                }
                else
                {
                    actionName = "Remove Connections";
                }
            }

            QAction* removeConnectionAction = menu.addAction(actionName.c_str());
            removeConnectionAction->setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/Remove.png"));
            connect(removeConnectionAction, SIGNAL(triggered()), this, SLOT(DeleteSelectedItems()));

            // show the menu at the given position
            menu.exec(globalMousePos);
        }
        else
        {
            OnContextMenuEvent(this, mousePos, globalMousePos, mPlugin, selectedNodes, true);
        }
    }


    // when double clicking
    void BlendGraphWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (mActiveGraph == nullptr)
        {
            return;
        }

        mDoubleClickHappened = true;
        //MCore::LogError("double click");
        //mMouseClickTimer->stop();
        NodeGraphWidget::mouseDoubleClickEvent(event);
        //mMouseClickTimer->stop();

        EMotionFX::AnimGraphNode* animGraphNode = FindFirstSelectedAnimGraphNode();
        if (animGraphNode)
        {
            if (animGraphNode->GetHasVisualGraph())
            {
                GraphNode* node = mActiveGraph->FindNode(event->pos());
                if (node && node->GetIsInsideArrowRect(mMousePos) == false)
                {
                    mPlugin->GetNavigateWidget()->ShowGraph(animGraphNode, true);
                    return;
                }
            }
        }

        event->accept();
    }


    void BlendGraphWidget::OnMouseClickTimeout()
    {
        if (mLastRightClick)
        {
            OnContextMenuEvent(mMouseLastPressPos, mLastGlobalMousePos);
        }
    }


    void BlendGraphWidget::mousePressEvent(QMouseEvent* event)
    {
        mDoubleClickHappened = false;
        //MCore::LogError("mouse press");
        mLastGlobalMousePos = event->globalPos();
        mLastRightClick = event->button() == Qt::RightButton;

        NodeGraphWidget::mousePressEvent(event);

        // mouse click and double click distinguishing data
        //const int doubleClickTimerinMS = QApplication::doubleClickInterval() + 50; // 500 ms is Qt's double click interval, we have to wait a bit longer
        //mMouseClickTimer->start(doubleClickTimerinMS);
    }


    void BlendGraphWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        //MCore::LogError("mouse release");

        if (mDoubleClickHappened == false)
        {
            if (event->button() == Qt::RightButton)
            {
                OnContextMenuEvent(event->pos(), event->globalPos());
                //setCursor( Qt::ArrowCursor );
            }
        }

        NodeGraphWidget::mouseReleaseEvent(event);
        //setCursor( Qt::ArrowCursor );
        mDoubleClickHappened = false;
    }


    // show nothing
    void BlendGraphWidget::ShowNothing()
    {
        // remove the active graph
        mActiveGraph = nullptr;
        mCurrentNode = nullptr;
    }


    // start moving
    void BlendGraphWidget::OnMoveStart()
    {
        mMoveGroup.RemoveAllCommands();
    }


    // moved a node
    void BlendGraphWidget::OnMoveNode(GraphNode* node, int32 x, int32 y)
    {
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // build the command string
        mMoveString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -xPos %d -yPos %d -updateAttributes false", animGraph->GetID(), node->GetName(), x, y);

        // add it to the group
        mMoveGroup.AddCommandString(mMoveString.c_str());
    }


    // end moving
    void BlendGraphWidget::OnMoveEnd()
    {
        AZStd::string resultString;

        // execute the command
        if (GetCommandManager()->ExecuteCommandGroup(mMoveGroup, resultString) == false)
        {
            if (resultString.size() > 0)
            {
                MCore::LogError(resultString.c_str());
            }
        }
    }



    // selected some nodes
    void BlendGraphWidget::OnSelectionChanged()
    {
        NavigateWidget* navigateWidget = mPlugin->GetNavigateWidget();
        QTreeWidget* treeWidget = navigateWidget->GetTreeWidget();

        // start block signals for the navigate tree widget
        treeWidget->blockSignals(true);
        {
            treeWidget->clearSelection();

            // iterate over all nodes
            const uint32 numNodes = mActiveGraph->GetNumNodes();
            for (uint32 i = 0; i < numNodes; ++i)
            {
                GraphNode* node = mActiveGraph->GetNode(i);
                if (node->GetIsSelected())
                {
                    QTreeWidgetItem* item = navigateWidget->FindItem(node->GetName());
                    if (item)
                    {
                        item->setSelected(true);
                    }
                }
            }

            //mPlugin->GetViewWidget()->Update();

#ifdef _DEBUG
            // When changing selection, the attribute window will be re-initialized.
            // An attribute will be constructed for each of the anim graph object attributes. Some of them being simple widgets
            // like a checkbox, but also more complex widgets like the blend space motions widget where we use icons etc.
            // In debug mode this operation can become slower than the double click interval. In case the operation triggered
            // with the first left click takes longer than the double click interval, the double click is not detected.
            // In order to fix this, we delay the attribute window re-init event until after the double click detection.
            // This will cause a delay in the interface and is not recommended for the release.
            QTimer::singleShot(300, mPlugin->GetNavigateWidget(), SLOT(OnSelectionChangedWithoutGraphUpdate()));
#else
            mPlugin->GetNavigateWidget()->OnSelectionChangedWithoutGraphUpdate();
#endif
        }
        // end block signals for the navigate tree widget
        treeWidget->blockSignals(false);
    }



    // check if a connection is valid or not
    bool BlendGraphWidget::CheckIfIsRelinkConnectionValid(NodeConnection* connection, GraphNode* newTargetNode, uint32 newTargetPortNr, bool isTargetInput)
    {
        GraphNode* targetNode = connection->GetSourceNode();
        GraphNode* sourceNode = newTargetNode;
        uint32 sourcePortNr = connection->GetOutputPortNr();
        uint32 targetPortNr = newTargetPortNr;

        // don't allow connection to itself
        if (sourceNode == targetNode)
        {
            return false;
        }

        // if we're not dealing with state nodes
        if (sourceNode->GetType() != StateGraphNode::TYPE_ID || targetNode->GetType() != StateGraphNode::TYPE_ID)
        {
            if (isTargetInput == false)
            {
                return false;
            }
        }

        // if this were states, it's all fine
        if (sourceNode->GetType() == StateGraphNode::TYPE_ID || targetNode->GetType() == StateGraphNode::TYPE_ID)
        {
            return true;
        }

        // check if there is already a connection in the port
        MCORE_ASSERT(sourceNode->GetType() == BlendTreeVisualNode::TYPE_ID);
        MCORE_ASSERT(targetNode->GetType() == BlendTreeVisualNode::TYPE_ID);
        BlendTreeVisualNode* targetBlendNode = static_cast<BlendTreeVisualNode*>(sourceNode);
        BlendTreeVisualNode* sourceBlendNode = static_cast<BlendTreeVisualNode*>(targetNode);

        EMotionFX::AnimGraphNode* emfxSourceNode = sourceBlendNode->GetEMFXNode();
        EMotionFX::AnimGraphNode* emfxTargetNode = targetBlendNode->GetEMFXNode();
        EMotionFX::AnimGraphNode::Port& sourcePort = emfxSourceNode->GetOutputPort(sourcePortNr);
        EMotionFX::AnimGraphNode::Port& targetPort = emfxTargetNode->GetInputPort(targetPortNr);

        // if the port data types are not compatible, don't allow the connection
        if (targetPort.CheckIfIsCompatibleWith(sourcePort) == false)
        {
            return false;
        }

        return true;
    }


    // check if a connection is valid or not
    bool BlendGraphWidget::CheckIfIsCreateConnectionValid(uint32 portNr, GraphNode* portNode, NodePort* port, bool isInputPort)
    {
        MCORE_UNUSED(port);
        MCORE_ASSERT(mActiveGraph);

        GraphNode* sourceNode = mActiveGraph->GetCreateConnectionNode();
        GraphNode* targetNode = portNode;

        // don't allow connection to itself
        if (sourceNode == targetNode)
        {
            return false;
        }

        // if we're not dealing with state nodes
        if (sourceNode->GetType() != StateGraphNode::TYPE_ID || targetNode->GetType() != StateGraphNode::TYPE_ID)
        {
            // dont allow to connect an input port to another input port or output port to another output port
            if (isInputPort == mActiveGraph->GetCreateConnectionIsInputPort())
            {
                return false;
            }
        }

        // if this were states, it's all fine
        if (sourceNode->GetType() == StateGraphNode::TYPE_ID || targetNode->GetType() == StateGraphNode::TYPE_ID)
        {
            return CheckIfIsValidTransition(sourceNode, targetNode);
        }

        // check if there is already a connection in the port
        MCORE_ASSERT(portNode->GetType() == BlendTreeVisualNode::TYPE_ID);
        MCORE_ASSERT(sourceNode->GetType() == BlendTreeVisualNode::TYPE_ID);
        BlendTreeVisualNode* targetBlendNode;
        BlendTreeVisualNode* sourceBlendNode;
        uint32 sourcePortNr;
        uint32 targetPortNr;

        // make sure the input always comes from the source node
        if (isInputPort)
        {
            sourceBlendNode = static_cast<BlendTreeVisualNode*>(sourceNode);
            targetBlendNode = static_cast<BlendTreeVisualNode*>(targetNode);
            sourcePortNr    = mActiveGraph->GetCreateConnectionPortNr();
            targetPortNr    = portNr;
        }
        else
        {
            sourceBlendNode = static_cast<BlendTreeVisualNode*>(targetNode);
            targetBlendNode = static_cast<BlendTreeVisualNode*>(sourceNode);
            sourcePortNr    = portNr;
            targetPortNr    = mActiveGraph->GetCreateConnectionPortNr();
        }

        EMotionFX::AnimGraphNode::Port& sourcePort = sourceBlendNode->GetEMFXNode()->GetOutputPort(sourcePortNr);
        EMotionFX::AnimGraphNode::Port& targetPort = targetBlendNode->GetEMFXNode()->GetInputPort(targetPortNr);

        // if the port data types are not compatible, don't allow the connection
        if (targetPort.CheckIfIsCompatibleWith(sourcePort) == false)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* parentNode = targetBlendNode->GetEMFXNode()->GetParentNode();
        EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(parentNode);

        if (blendTree->ConnectionWillProduceCycle(sourceBlendNode->GetEMFXNode(), targetBlendNode->GetEMFXNode()))
        {
            return false;
        }

        return true;
    }

    bool BlendGraphWidget::CheckIfIsValidTransition(GraphNode* sourceState, GraphNode* targetState)
    {
        if (azrtti_typeid(static_cast<AnimGraphVisualNode*>(sourceState)->GetEMFXNode()) == azrtti_typeid<EMotionFX::AnimGraphExitNode>())
        {
            return false;
        }

        return true;
    }

    bool BlendGraphWidget::CheckIfIsValidTransitionSource(GraphNode* sourceState)
    {
        if (azrtti_typeid(static_cast<AnimGraphVisualNode*>(sourceState)->GetEMFXNode()) == azrtti_typeid<EMotionFX::AnimGraphExitNode>())
        {
            return false;
        }

        return true;
    }

    EMotionFX::AnimGraphNode* BlendGraphWidget::FindFirstSelectedAnimGraphNode()
    {
        EMotionFX::AnimGraph*  animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return nullptr;
        }

        NodeGraph* nodeGraph = GetActiveGraph();
        if (nodeGraph == nullptr)
        {
            return nullptr;
        }

        GraphNode* selectedNode = nodeGraph->FindFirstSelectedGraphNode();
        if (selectedNode == nullptr)
        {
            //LogWarning("No node selected. Please select a node first.");
            return nullptr;
        }

        return animGraph->RecursiveFindNodeByName(selectedNode->GetName());
    }


    EMotionFX::AnimGraphStateTransition* BlendGraphWidget::FindTransitionForConnection(NodeConnection* connection)
    {
        if (connection == nullptr || mCurrentNode == nullptr)
        {
            return nullptr;
        }

        MCORE_ASSERT(mPlugin->GetActiveAnimGraph());

        if (azrtti_typeid(mCurrentNode) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            return nullptr;
        }

        EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(mCurrentNode);
        return stateMachine->FindTransitionByID(connection->GetID());
    }


    EMotionFX::BlendTreeConnection* BlendGraphWidget::FindBlendTreeConnection(NodeConnection* connection) const
    {
        // Make sure the given connection is valid and the currently viewed graph is a blend tree.
        if (!connection || !mCurrentNode || azrtti_typeid(mCurrentNode) != azrtti_typeid<EMotionFX::BlendTree>())
        {
            return nullptr;
        }

        const EMotionFX::AnimGraph* animGraph = mCurrentNode->GetAnimGraph();
        if (!animGraph)
        {
            return nullptr;
        }

        const GraphNode* sourceNode = connection->GetSourceNode();
        if (!sourceNode)
        {
            return nullptr;
        }

        const EMotionFX::AnimGraphNode* emfxSourceNode = animGraph->RecursiveFindNodeById(sourceNode->GetId());
        if (!emfxSourceNode)
        {
            return nullptr;
        }        

        const EMotionFX::BlendTree* blendTree = static_cast<EMotionFX::BlendTree*>(mCurrentNode);

        // Iterate over all nodes in the blend tree and check if the given visual connection represents the blend tree connection we're searching.
        const uint32 numChildNodes = blendTree->GetNumChildNodes();
        for (uint32 i = 0; i < numChildNodes; ++i)
        {
            const EMotionFX::AnimGraphNode* childNode = blendTree->GetChildNode(i);

            EMotionFX::BlendTreeConnection* emfxConnection = childNode->FindConnection(emfxSourceNode, connection->GetOutputPortNr(), connection->GetInputPortNr());
            if (emfxConnection)
            {
                return emfxConnection;
            }
        }

        return nullptr;
    }


    // create the connection
    void BlendGraphWidget::OnCreateConnection(uint32 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, uint32 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset)
    {
        MCORE_UNUSED(targetIsInputPort);
        MCORE_ASSERT(mActiveGraph);

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        GraphNode*  realSourceNode;
        GraphNode*  realTargetNode;
        uint32      realInputPortNr;
        uint32      realOutputPortNr;

        if (sourceIsInputPort)
        {
            realSourceNode      = targetNode;
            realTargetNode      = sourceNode;
            realOutputPortNr    = targetPortNr;
            realInputPortNr     = sourcePortNr;
        }
        else
        {
            realSourceNode      = sourceNode;
            realTargetNode      = targetNode;
            realOutputPortNr    = sourcePortNr;
            realInputPortNr     = targetPortNr;
        }

        MCore::CommandGroup commandGroup;
        AZStd::string command;

        // Check if there already is a connection plugged into the port where we want to put our new connection in.
        NodeConnection* existingConnection = mActiveGraph->FindInputConnection(realTargetNode, realInputPortNr);

        // Special case for state nodes.
        AZ::TypeId transitionType = AZ::TypeId::CreateNull();
        if (sourceNode->GetType() == StateGraphNode::TYPE_ID && targetNode->GetType() == StateGraphNode::TYPE_ID)
        {
            transitionType      = azrtti_typeid<EMotionFX::AnimGraphStateTransition>();
            realInputPortNr     = 0;
            realOutputPortNr    = 0;
            commandGroup.SetGroupName("Create state machine transition");
        }
        else
        {
            // Check if there already is a connection and remove it in this case.
            if (existingConnection)
            {
                commandGroup.SetGroupName("Replace blend tree connection");

                command = AZStd::string::format("AnimGraphRemoveConnection -animGraphID %i -sourceNode \"%s\" -sourcePort %d -targetNode \"%s\" -targetPort %d",
                        animGraph->GetID(),
                        existingConnection->GetSourceNode()->GetName(),
                        existingConnection->GetOutputPortNr(),
                        existingConnection->GetTargetNode()->GetName(),
                        existingConnection->GetInputPortNr());

                commandGroup.AddCommandString(command);
            }
            else
            {
                commandGroup.SetGroupName("Create blend tree connection");
            }
        }


        if (transitionType.IsNull())
        {
            command = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d", animGraph->GetID(), realSourceNode->GetName(), realTargetNode->GetName(), realOutputPortNr, realInputPortNr, startOffset.x(), startOffset.y(), endOffset.x(), endOffset.y());
        }
        else
        {
            command = AZStd::string::format("AnimGraphCreateConnection -animGraphID %i -sourceNode \"%s\" -targetNode \"%s\" -sourcePort %d -targetPort %d -startOffsetX %d -startOffsetY %d -endOffsetX %d -endOffsetY %d -transitionType \"%s\"", animGraph->GetID(), realSourceNode->GetName(), realTargetNode->GetName(), realOutputPortNr, realInputPortNr, startOffset.x(), startOffset.y(), endOffset.x(), endOffset.y(), transitionType.ToString<AZStd::string>().c_str());
        }

        commandGroup.AddCommandString(command);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Send LyMetrics event.
        MetricsEventSender::SendCreateConnectionEvent(!transitionType.IsNull());
    }


    // curved connection when creating a new one?
    bool BlendGraphWidget::CreateConnectionMustBeCurved()
    {
        if (mActiveGraph == nullptr)
        {
            return true;
        }

        if (mActiveGraph->GetCreateConnectionNode()->GetType() == StateGraphNode::TYPE_ID)
        {
            return false;
        }

        return true;
    }


    // show helper connection suggestion lines when creating a new connection?
    bool BlendGraphWidget::CreateConnectionShowsHelpers()
    {
        if (mActiveGraph == nullptr)
        {
            return true;
        }

        if (mActiveGraph->GetCreateConnectionNode()->GetType() == StateGraphNode::TYPE_ID)
        {
            return false;
        }

        return true;
    }


    // delete all selected items
    void BlendGraphWidget::DeleteSelectedItems()
    {
        DeleteSelectedItems(mActiveGraph);
    }


    void BlendGraphWidget::DeleteSelectedItems(NodeGraph* nodeGraph)
    {
        if (!nodeGraph)
        {
            return;
        }

        // Do not allow to delete nodes or connections when creating or relinking connections or transitions.
        // In this case the delete operation will cancel the create or relink operation.
        if (nodeGraph->GetIsCreatingConnection() || nodeGraph->GetIsRelinkingConnection() ||
            nodeGraph->GetRepositionedTransitionHead() || nodeGraph->GetRepositionedTransitionTail()) 
        {
            nodeGraph->StopCreateConnection();
            nodeGraph->StopRelinkConnection();
            nodeGraph->StopReplaceTransitionHead();
            nodeGraph->StopReplaceTransitionTail();
            return;
        }

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Delete selected anim graph items");

        AZStd::vector<EMotionFX::BlendTreeConnection*> connectionList;
        AZStd::vector<EMotionFX::AnimGraphStateTransition*> transitionList;
        AZStd::vector<EMotionFX::AnimGraphNode*> nodeList;
        connectionList.reserve(256);
        transitionList.reserve(256);
        nodeList.reserve(256);

        // Delete all selected connections in the graph view first.
        AZStd::string commandString, sourceNodeName;
        AZ::u32 numDeletedConnections = 0;
        const uint32 numNodes = nodeGraph->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = nodeGraph->GetNode(i);
            const uint32 numConnections = node->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
            {
                NodeConnection* connection = node->GetConnection(c);
                if (connection->GetIsSelected())
                {
                    EMotionFX::AnimGraphStateTransition* emfxTransition = FindTransitionForConnection(connection);
                    if (emfxTransition)
                    {
                        CommandSystem::DeleteStateTransition(&commandGroup, emfxTransition, transitionList);
                        numDeletedConnections++;
                    }
                    else
                    {
                        EMotionFX::BlendTreeConnection* emfxConnection = FindBlendTreeConnection(connection);
                        EMotionFX::AnimGraphNode* emfxTargetNode = animGraph->RecursiveFindNodeById(connection->GetTargetNode()->GetId());

                        if (emfxConnection && emfxTargetNode)
                        {
                            CommandSystem::DeleteConnection(&commandGroup, emfxTargetNode, emfxConnection, connectionList);
                            numDeletedConnections++;
                        }
                    }
                }
            }
        }

        // Prepare the list of nodes to remove.
        AZStd::vector<AZStd::string> selectedNodeNames;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            GraphNode* node = nodeGraph->GetNode(i);
            if (!node->GetIsSelected())
            {
                continue;
            }

            selectedNodeNames.push_back(node->GetName());
        }

        CommandSystem::DeleteNodes(&commandGroup, animGraph, selectedNodeNames, nodeList, connectionList, transitionList);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Send LyMetrics event.
        MetricsEventSender::SendDeleteNodesAndConnectionsEvent(static_cast<AZ::u32>(selectedNodeNames.size()), numDeletedConnections);
    }


    // some node collapsed
    void BlendGraphWidget::OnNodeCollapsed(GraphNode* node, bool isCollapsed)
    {
        if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            blendNode->GetEMFXNode()->SetIsCollapsed(isCollapsed);
        }
    }


    // left-clicked on a node while shift pressed
    void BlendGraphWidget::OnShiftClickedNode(GraphNode* node)
    {
        // when we are dealing with a state node
        if (node->GetType() == StateGraphNode::TYPE_ID)
        {
            //EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();

            // for all selected actor instances
            CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            const uint32 numActorInstances = selection.GetNumSelectedActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

                // skip actor instances not using the new blending system
                if (animGraphInstance == nullptr)
                {
                    continue;
                }

                animGraphInstance->TransitionToState(node->GetName());
            }
        }
    }


    void BlendGraphWidget::OnNodeGroupSelected()
    {
        assert(sender()->inherits("QAction"));
        QAction* action = qobject_cast<QAction*>(sender());

        // get the anim graph
        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        // find the selected node
        MCore::Array<EMotionFX::AnimGraphNode*> selectedNodes = mPlugin->GetNavigateWidget()->GetSelectedNodes(animGraph);
        if (selectedNodes.GetIsEmpty())
        {
            return;
        }

        // get the node group name from the action and search the node group
        EMotionFX::AnimGraphNodeGroup* newNodeGroup;
        if (action->data().toInt() == 0)
        {
            newNodeGroup = nullptr;
        }
        else
        {
            const AZStd::string nodeGroupName = FromQtString(action->text());
            newNodeGroup = animGraph->FindNodeGroupByName(nodeGroupName.c_str());
        }

        // create the command group
        MCore::CommandGroup commandGroup("Adjust anim graph node group");
        AZStd::string tempString;

        // add each command
        if (newNodeGroup)
        {
            // add each "remove" command
            const uint32 numSelectedNodes = selectedNodes.GetLength();
            for (uint32 i = 0; i < numSelectedNodes; ++i)
            {
                EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(selectedNodes[i]);
                if (nodeGroup)
                {
                    tempString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -nodeNames \"%s\" -nodeAction \"remove\"", animGraph->GetID(), nodeGroup->GetName(), selectedNodes[i]->GetName());
                    commandGroup.AddCommandString(tempString.c_str());
                }
            }

            // build the node names
            AZStd::string nodeNames;
            for (uint32 i = 0; i < numSelectedNodes; ++i)
            {
                nodeNames += selectedNodes[i]->GetName();
                if (i < numSelectedNodes - 1)
                {
                    nodeNames += ';';
                }
            }

            // add the "add" command
            tempString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -nodeNames \"%s\" -nodeAction \"add\"", animGraph->GetID(), newNodeGroup->GetName(), nodeNames.c_str());
            commandGroup.AddCommandString(tempString.c_str());
        }
        else
        {
            const uint32 numSelectedNodes = selectedNodes.GetLength();
            for (uint32 i = 0; i < numSelectedNodes; ++i)
            {
                EMotionFX::AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupForNode(selectedNodes[i]);
                if (nodeGroup)
                {
                    tempString = AZStd::string::format("AnimGraphAdjustNodeGroup -animGraphID %i -name \"%s\" -nodeNames \"%s\" -nodeAction \"remove\"", animGraph->GetID(), nodeGroup->GetName(), selectedNodes[i]->GetName());
                    commandGroup.AddCommandString(tempString.c_str());
                }
            }
        }

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, tempString) == false)
        {
            MCore::LogError(tempString.c_str());
        }
    }


    bool BlendGraphWidget::PreparePainting()
    {
        // skip rendering in case rendering is disabled
        if (mPlugin->GetDisableRendering())
        {
            return false;
        }

        if (mActiveGraph)
        {
            if (mActiveGraph->GetRenderNodeGroupsCallback() == nullptr)
            {
                mActiveGraph->SetRenderNodeGroupsCallback(new BlendGraphRenderNodeGroupsCallback(mPlugin, this));
            }

            // enable or disable graph animation
            mActiveGraph->SetUseAnimation(mPlugin->GetAnimGraphOptions().GetGraphAnimation());
        }

        // pass down the show fps options flag
        NodeGraphWidget::SetShowFPS(mPlugin->GetAnimGraphOptions().GetShowFPS());

        return true;
    }


    // enable or disable node visualization
    void BlendGraphWidget::OnVisualizeToggle(GraphNode* node, bool visualizeEnabled)
    {
        //if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            blendNode->GetEMFXNode()->SetVisualization(visualizeEnabled);
        }
    }


    // enable or disable the node
    void BlendGraphWidget::OnEnabledToggle(GraphNode* node, bool enabled)
    {
        if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            blendNode->GetEMFXNode()->SetIsEnabled(enabled);
        }
    }


    // change visualize options
    void BlendGraphWidget::OnSetupVisualizeOptions(GraphNode* node)
    {
        //if (node->GetType() == BlendTreeVisualNode::TYPE_ID)
        {
            BlendTreeVisualNode* blendNode = static_cast<BlendTreeVisualNode*>(node);
            //blendNode->GetEMFXNode()->SetVisualization( visualizeEnabled );
            //MCore::LogInfo("Settings for node '%s'", node->GetName());
            mPlugin->GetNavigateWidget()->SetVisualOptionsNode(blendNode->GetEMFXNode());
            mPlugin->GetNavigateWidget()->OnVisualizeOptions();
            mPlugin->GetNavigateWidget()->SetVisualOptionsNode(nullptr);
        }
    }


    bool BlendGraphWidget::event(QEvent* event)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

            QFontMetrics fontMetrics(QToolTip::font());

            QFont boldFont = QToolTip::font();
            boldFont.setBold(true);
            QFontMetrics boldFontMetrics(boldFont);

            if (mActiveGraph)
            {
                AZStd::string toolTipString;

                QPoint localPos     = helpEvent->pos();
                QPoint globalPos    = LocalToGlobal(localPos);
                QPoint tooltipPos   = helpEvent->globalPos();

                // find the connection at the mouse position
                NodeConnection* connection = mActiveGraph->FindConnection(globalPos);
                if (connection)
                {
                    bool conditionFound = false;
                    if (connection->GetType() == StateConnection::TYPE_ID)
                    {
                        StateConnection* stateConnection = static_cast<StateConnection*>(connection);
                        EMotionFX::AnimGraphTransitionCondition* condition = stateConnection->FindCondition(globalPos);
                        if (condition)
                        {
                            AZStd::string tempConditionString;
                            condition->GetTooltip(&tempConditionString);

                            toolTipString = "<qt>";
                            toolTipString += tempConditionString;
                            toolTipString += "</qt>";

                            conditionFound = true;
                        }
                    }

                    // get the output and the input port numbers
                    const uint32 outputPortNr   = connection->GetOutputPortNr();
                    const uint32 inputPortNr    = connection->GetInputPortNr();

                    // show connection or state transition tooltip
                    if (conditionFound == false)
                    {
                        GraphNode* sourceNode = connection->GetSourceNode();
                        GraphNode* targetNode = connection->GetTargetNode();

                        // prepare the colors
                        QColor sourceColor, targetColor;
                        if (sourceNode)
                        {
                            sourceColor = sourceNode->GetBaseColor();
                        }
                        if (targetNode)
                        {
                            targetColor = targetNode->GetBaseColor();
                        }

                        // prepare the node names
                        AZStd::string sourceNodeName, targetNodeName;
                        if (sourceNode)
                        {
                            sourceNodeName = sourceNode->GetName();
                        }
                        if (targetNode)
                        {
                            targetNodeName = targetNode->GetName();
                        }
                        //sourceNodeName.ConvertToNonBreakingHTMLSpaces();
                        //targetNodeName.ConvertToNonBreakingHTMLSpaces();

                        // check if we are dealing with a node inside a blend tree
                        if (sourceNode && sourceNode->GetType() == BlendTreeVisualNode::TYPE_ID)
                        {
                            // type cast it to a blend graph node and get the corresponding emfx node
                            BlendTreeVisualNode* blendSourceNode = static_cast<BlendTreeVisualNode*>(sourceNode);
                            EMotionFX::AnimGraphNode* sourceEMFXNode = blendSourceNode->GetEMFXNode();

                            // prepare the port names
                            AZStd::string outputPortName, inputPortName;
                            if (sourceNode)
                            {
                                outputPortName = sourceNode->GetOutputPort(outputPortNr)->GetName();
                            }
                            if (targetNode)
                            {
                                inputPortName = targetNode->GetInputPort(inputPortNr)->GetName();
                            }
                            //outputPortName.ConvertToNonBreakingHTMLSpaces();
                            //inputPortName.ConvertToNonBreakingHTMLSpaces();

                            int columnSourceWidth = boldFontMetrics.width(sourceNodeName.c_str()) + boldFontMetrics.width(" ") + fontMetrics.width("(Port: ") +  fontMetrics.width(outputPortName.c_str()) + fontMetrics.width(")");
                            int columnTargetWidth = boldFontMetrics.width(targetNodeName.c_str()) + boldFontMetrics.width(" ") + fontMetrics.width("(Port: ") +  fontMetrics.width(inputPortName.c_str()) + fontMetrics.width(")");

                            // construct the html tooltip string
                            toolTipString += AZStd::string::format("<qt><table border=\"0\"><tr><td width=\"%i\"><p style=\"color:rgb(%i,%i,%i)\"><b>%s </b>(Port: %s)</p></td> <td>to</td> <td width=\"%i\"><p style=\"color:rgb(%i,%i,%i)\"><b>%s </b>(Port: %s)</p></td></tr>", columnSourceWidth, sourceColor.red(), sourceColor.green(), sourceColor.blue(), sourceNodeName.c_str(), outputPortName.c_str(), columnTargetWidth, targetColor.red(), targetColor.green(), targetColor.blue(), targetNodeName.c_str(), inputPortName.c_str());

                            // now check if the connection is coming from a parameter node
                            if (azrtti_typeid(sourceEMFXNode) == azrtti_typeid<EMotionFX::BlendTreeParameterNode>())
                            {
                                EMotionFX::BlendTreeParameterNode* parameterNode = static_cast<EMotionFX::BlendTreeParameterNode*>(sourceEMFXNode);

                                // get the parameter index from the port where the connection starts
                                uint32 parameterIndex = parameterNode->GetParameterIndex(outputPortNr);
                                if (parameterIndex != MCORE_INVALIDINDEX32)
                                {
                                    // get access to the parameter name and add it to the tool tip
                                    EMotionFX::AnimGraph* animGraph = parameterNode->GetAnimGraph();
                                    const EMotionFX::Parameter* parameter = animGraph->FindValueParameter(parameterIndex);

                                    toolTipString += "\n<qt><table border=\"0\"><tr>";
                                    toolTipString += AZStd::string::format("<td><p style=\"color:rgb(80, 80, 80)\"><b>Parameter:</b></p></td><td><p style=\"color:rgb(115, 115, 115)\">%s</p></td>", parameter->GetName().c_str());
                                    toolTipString += "</tr></table></qt>";
                                }
                            }
                        }
                        // state machine node
                        else
                        {
                            toolTipString = "<qt><table><tr>";

                            // construct the html tooltip string
                            if (sourceNode && targetNode)
                            {
                                toolTipString += AZStd::string::format("<td width=\"%i\"><b><p style=\"color:rgb(%i,%i,%i)\">%s</p></b></td> <td>to</td> <td width=\"%i\"><b><nobr><p style=\"color:rgb(%i,%i,%i)\">%s</p></nobr></b></td>", boldFontMetrics.width(sourceNodeName.c_str()), sourceColor.red(), sourceColor.green(), sourceColor.blue(), sourceNodeName.c_str(), boldFontMetrics.width(targetNodeName.c_str()), targetColor.red(), targetColor.green(), targetColor.blue(), targetNodeName.c_str());
                            }
                            else if (targetNode)
                            {
                                toolTipString += AZStd::string::format("<td>to</td> <td width=\"%i\"><b><p style=\"color:rgb(%i,%i,%i)\">%s</p></b></td>", boldFontMetrics.width(targetNodeName.c_str()), targetColor.red(), targetColor.green(), targetColor.blue(), targetNodeName.c_str());
                            }

                            toolTipString += "</tr></table></qt>";
                        }
                    }
                }

                GraphNode*                  node            = mActiveGraph->FindNode(localPos);
                EMotionFX::AnimGraphNode*  animGraphNode  = nullptr;

                if (node)
                {
                    BlendTreeVisualNode* blendNode   = static_cast<BlendTreeVisualNode*>(node);
                    animGraphNode              = blendNode->GetEMFXNode();

                    AZStd::string tempString;
                    toolTipString = "<qt><table border=\"0\">";

                    // node name
                    toolTipString += AZStd::string::format("<tr><td><b>Name:</b></td><td><nobr>%s</nobr></td></tr>", animGraphNode->GetName());

                    // node palette name
                    toolTipString += AZStd::string::format("<tr><td><b>Type:</b></td><td><nobr>%s</nobr></td></tr>", animGraphNode->GetPaletteName());

                    if (animGraphNode->GetCanHaveChildren())
                    {
                        // child nodes
                        toolTipString += AZStd::string::format("<tr><td><b><nobr>Child Nodes:</nobr></b></td><td>%i</td></tr>", animGraphNode->GetNumChildNodes());

                        // recursive child nodes
                        toolTipString += AZStd::string::format("<tr><td width=\"140\"><b><nobr>Recursive Child Nodes:</nobr></b></td><td>%i</td></tr>", animGraphNode->RecursiveCalcNumNodes());
                    }

                    // states
                    if (node->GetType() == StateGraphNode::TYPE_ID)
                    {
                        // get access to the state machine
                        EMotionFX::AnimGraphStateMachine* stateMachine = nullptr;
                        EMotionFX::AnimGraphNode* parentNode = animGraphNode->GetParentNode();
                        if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                        {
                            stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                        }

                        // incoming transitions
                        toolTipString += AZStd::string::format("<tr><td><b>Incoming Transitions:</b></td><td>%i</td></tr>", stateMachine->CalcNumIncomingTransitions(animGraphNode));

                        // outgoing transitions
                        toolTipString += AZStd::string::format("<tr><td width=\"130\"><b>Outgoing Transitions:</b></td><td>%i</td></tr>", stateMachine->CalcNumOutgoingTransitions(animGraphNode));
                    }

                    // complete the table
                    toolTipString += "</table></qt>";
                }

                if (!toolTipString.empty())
                {
                    QRect toolTipRect(globalPos.x() - 4, globalPos.y() - 4, 8, 8);
                    QToolTip::showText(tooltipPos, toolTipString.c_str(), this, toolTipRect);
                }

                return NodeGraphWidget::event(event);
            }
        }

        return NodeGraphWidget::event(event);
    }


    void BlendGraphWidget::ReplaceTransition(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode)
    {
        if (mCurrentNode == nullptr || connection->GetType() != StateConnection::TYPE_ID)
        {
            return;
        }

        EMotionFX::AnimGraph* animGraph = mPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            return;
        }

        StateConnection* stateConnection = static_cast<StateConnection*>(connection);

        AZStd::string commandString;
        commandString.reserve(2048);

        // get the state machine name and the transition id to work on
        AZStd::string stateMachineName = mCurrentNode->GetName();
        const uint32 transitionID = connection->GetID();

        // construct the command including the required parameters
        commandString = AZStd::string::format("AnimGraphAdjustConnection -animGraphID %i -stateMachine \"%s\" -transitionID %i -startOffsetX %i -startOffsetY %i -endOffsetX %i -endOffsetY %i ", animGraph->GetID(), stateMachineName.c_str(), transitionID, stateConnection->GetStartOffset().x(), stateConnection->GetStartOffset().y(), stateConnection->GetEndOffset().x(), stateConnection->GetEndOffset().y());

        if (stateConnection->GetSourceNode())
        {
            commandString += AZStd::string::format("-sourceNode \"%s\" ", stateConnection->GetSourceNode()->GetName());
        }
        if (stateConnection->GetTargetNode())
        {
            commandString += AZStd::string::format("-targetNode \"%s\" ", stateConnection->GetTargetNode()->GetName());
        }

        mActiveGraph->StopReplaceTransitionHead();
        mActiveGraph->StopReplaceTransitionTail();

        // reset the visual transition after we got the command constructed
        stateConnection->SetSourceNode(sourceNode);
        stateConnection->SetTargetNode(targetNode);
        stateConnection->SetStartOffset(startOffset);
        stateConnection->SetEndOffset(endOffset);

        if (mActiveGraph->GetReplaceTransitionValid())
        {
            AZStd::string outResult;
            if (GetCommandManager()->ExecuteCommand(commandString.c_str(), outResult, true) == false)
            {
                if (outResult.empty() == false)
                {
                    MCore::LogError(outResult.c_str());
                }
            }
        }
    }


    // on keypress
    void BlendGraphWidget::keyPressEvent(QKeyEvent* event)
    {
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        if (shortcutManager->Check(event, "Open Parent Node", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->NavigateToParent();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Open Selected Node", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->NavigateToNode();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "History Back", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->NavigateBackward();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "History Forward", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->NavigateForward();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Align Left", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->AlignLeft();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Align Right", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->AlignRight();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Align Top", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->AlignTop();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Align Bottom", "Anim Graph Window"))
        {
            mPlugin->GetViewWidget()->AlignBottom();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Cut", "Anim Graph Window"))
        {
            NavigateWidget* navigateWidget = mPlugin->GetNavigateWidget();
            navigateWidget->Cut();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Copy", "Anim Graph Window"))
        {
            NavigateWidget* navigateWidget = mPlugin->GetNavigateWidget();
            navigateWidget->Copy();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Paste", "Anim Graph Window"))
        {
            NavigateWidget* navigateWidget = mPlugin->GetNavigateWidget();
            navigateWidget->Paste();
            event->accept();
            return;
        }

        if (shortcutManager->Check(event, "Select All", "Anim Graph Window"))
        {
            if (mActiveGraph)
            {
                mActiveGraph->SelectAllNodes(true);
                event->accept();
            }
            return;
        }

        if (shortcutManager->Check(event, "Unselect All", "Anim Graph Window"))
        {
            if (mActiveGraph)
            {
                mActiveGraph->UnselectAllNodes(true);
                event->accept();
            }
            return;
        }

        if (shortcutManager->Check(event, "Delete Selected Nodes", "Anim Graph Window"))
        {
            DeleteSelectedItems();
            event->accept();
            return;
        }

        return NodeGraphWidget::keyPressEvent(event);
    }


    // on key release
    void BlendGraphWidget::keyReleaseEvent(QKeyEvent* event)
    {
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        if (shortcutManager->Check(event, "Open Parent Node", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Open Selected Node", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "History Back", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "History Forward", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Align Left", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Align Right", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Align Top", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Align Bottom", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Cut", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Copy", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Paste", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Select All", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Unselect All", "Anim Graph Window"))
        {
            event->accept();
            return;
        }
        if (shortcutManager->Check(event, "Delete Selected Nodes", "Anim Graph Window"))
        {
            event->accept();
            return;
        }

        return NodeGraphWidget::keyReleaseEvent(event);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.moc>
