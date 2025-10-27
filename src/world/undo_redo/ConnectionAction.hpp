#pragma once

#include <string>
#include <set>
#include <memory>
#include "IAction.hpp"
#include "../Connection.hpp"
#include "../Room.hpp"
#include "../Globals.hpp"

namespace UndoRedo {

    struct ConnectionData {
        Room* roomA;
        unsigned int connectionA;
        Room* roomB;
        unsigned int connectionB;
        std::set<std::string> timelines;
        TimelineType timelineType;

        ConnectionData()
            : roomA(nullptr), connectionA(0), roomB(nullptr), connectionB(0), timelineType(TimelineType::ALL) {}

        explicit ConnectionData(Connection* conn)
            : roomA(conn->roomA),
              connectionA(conn->connectionA),
              roomB(conn->roomB),
              connectionB(conn->connectionB),
              timelines(conn->timelines),
              timelineType(conn->timelineType) {}
    };

    enum class ConnectionActionType {
        Connect,
        Disconnect
    };

    class ConnectionAction : public IAction {
    private:
        ConnectionData data;
        ConnectionActionType actionType;
        void reconstructConnection();
        void destroyConnection();

    public:
        ConnectionAction(const ConnectionData& connectionData, ConnectionActionType actionType);

        void undo() override;
        void redo() override;
        std::string getDescription() const override;
        std::string toString() const override { return "Connection"; }
    };

    void pushConnectionSnapshot(Connection* connection, ConnectionActionType actionType);
}
