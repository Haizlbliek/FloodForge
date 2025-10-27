#include "ConnectionAction.hpp"
#include "UndoRedoManager.hpp"
#include "../Room.hpp"
#include "../../Logger.hpp"
#include "../Globals.hpp"

namespace UndoRedo {

    ConnectionAction::ConnectionAction(const ConnectionData& connectionData, ConnectionActionType actionType)
        : data(connectionData), actionType(actionType) {}

    std::string ConnectionAction::getDescription() const {
        return (actionType == ConnectionActionType::Disconnect)
            ? "Disconnect Rooms"
            : "Connect Rooms";
    }

    void ConnectionAction::undo() {
        if (!data.roomA || !data.roomB) return;

        if (actionType == ConnectionActionType::Disconnect)
            reconstructConnection();
        else
            destroyConnection();
    }

    void ConnectionAction::redo() {
        if (!data.roomA || !data.roomB) return;

        if (actionType == ConnectionActionType::Disconnect)
            destroyConnection();
        else
            reconstructConnection();
    }


    void pushConnectionSnapshot(Connection* connection, ConnectionActionType actionType) {
        if (!connection) return;
        ConnectionData snapshot(connection);

        auto action = std::make_shared<ConnectionAction>(snapshot, actionType);
        manager.pushAction(action);
    }
    

    void ConnectionAction::reconstructConnection() {
        auto* newConn = new Connection(
            data.roomA, data.connectionA,
            data.roomB, data.connectionB
        );
        newConn->timelines = data.timelines;
        newConn->timelineType = data.timelineType;

        EditorState::connections.push_back(newConn);
        data.roomA->connect(newConn);
        data.roomB->connect(newConn);

        Logger::info("[UndoRedo] Connection reconstructed");
    }

    void ConnectionAction::destroyConnection() {
        for (auto* conn : data.roomA->connections) {
            if ((conn->roomA == data.roomA && conn->roomB == data.roomB) ||
                (conn->roomA == data.roomB && conn->roomB == data.roomA)) {

                EditorState::connections.erase(
                    std::remove(EditorState::connections.begin(), EditorState::connections.end(), conn),
                    EditorState::connections.end()
                );
                data.roomA->disconnect(conn);
                data.roomB->disconnect(conn);
                delete conn;

                Logger::info("[UndoRedo] Connection destroyed");
                break;
            }
        }
    }

    
}
