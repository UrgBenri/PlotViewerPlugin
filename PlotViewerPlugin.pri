!contains( included_modules, $$PWD ) {
    included_modules += $$PWD
    QT += core gui

    !include($$PWD/PluginInterface/UrgBenriPluginInterface.pri) {
            error("Unable to include Viewer Plugin Interface.")
    }

    !include($$PWD/UrgDrawWidget/UrgDrawWidget.pri) {
            error("Unable to include Draw Widget.")
    }

    !include($$PWD/ColorButton/ColorButton.pri) {
            error("Unable to include Color Button.")
    }


    DEPENDPATH += $$PWD
    INCLUDEPATH += $$PWD

    SOURCES += \
            $$PWD/PlotViewerPlugin.cpp \
            $$PWD/PlotSettingsWidget.cpp \
            $$PWD/MathUtils.cpp

    HEADERS  += \
            $$PWD/PlotViewerPlugin.h \
            $$PWD/PlotSettingsWidget.h \
            $$PWD/MovingAverage.h \
            $$PWD/MathUtils.h

    FORMS += \
            $$PWD/PlotViewerPlugin.ui \
            $$PWD/PlotSettingsWidget.ui

    RESOURCES += \
            $$PWD/PlotViewerPlugin.qrc

    TRANSLATIONS = $$PWD/i18n/plugin_fr.ts \
            $$PWD/i18n/plugin_en.ts \
            $$PWD/i18n/plugin_ja.ts
}
