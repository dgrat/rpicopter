#ifndef Q_ATTITUDEDOCK
#define Q_ATTITUDEDOCK

#include <QtWidgets>
#include "attitudeindicator.h"



class QAttitudeDockWidget : public QDockWidget
{
private:
    qAttitudeIndicator m_AttitudeIndicator;

    void Setup() {
        this->setWidget(&m_AttitudeIndicator);
        this->setMaximumWidth(204);
    }


public:
    QAttitudeDockWidget(const QString & title, QWidget * parent = 0, Qt::WindowFlags flags = 0) : QDockWidget(title, parent, flags) {
        Setup();
    }

    QAttitudeDockWidget (QWidget * parent = 0, Qt::WindowFlags flags = 0 )  : QDockWidget(parent, flags) {
        Setup();
    }

    qAttitudeIndicator *GetIndicator() {
        return &m_AttitudeIndicator;
    }
};

#endif
