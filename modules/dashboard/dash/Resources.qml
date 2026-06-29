pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Caelestia.Config
import Caelestia.Services
import qs.components
import qs.components.controls
import qs.services

Item {
    id: root

    anchors.top: parent.top
    anchors.bottom: parent.bottom

    implicitWidth: layout.implicitWidth + layout.anchors.margins * 2

    ServiceRef {
        service: Cpu
    }

    ServiceRef {
        service: Memory
    }

    ServiceRef {
        service: Storage
    }

    ColumnLayout {
        id: layout

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Tokens.padding.large
        spacing: Tokens.spacing.medium

        Resource {
            icon: "memory"
            rawValue: Cpu.percentage
        }

        Resource {
            icon: "memory_alt"
            rawValue: Memory.percentage
            fgColour: Colours.palette.m3tertiary
        }

        Resource {
            icon: "hard_disk"
            rawValue: Storage.percentage
            fgColour: Colours.palette.m3secondary
        }
    }
    component Resource: CircularProgress {
        id: res

        required property string icon
        required property real rawValue
        readonly property real targetValue: Math.max(0, Math.min(1, isNaN(rawValue) ? 0 : rawValue))
        property bool animateNextValueChange
        property real displayedValue: targetValue

        Layout.fillHeight: true
        implicitSize: height
        strokeWidth: Tokens.sizes.dashboard.resourceProgressThickness
        value: displayedValue

        onTargetValueChanged: {
            animateNextValueChange = Math.abs(targetValue - displayedValue) >= 0.05;
            displayedValue = targetValue;
        }

        Behavior on displayedValue {
            enabled: res.animateNextValueChange

            Anim {}
        }

        MaterialIcon {
            anchors.centerIn: parent
            text: res.icon
            font: Tokens.font.icon.large
            color: res.fgColour
        }
    }
}
