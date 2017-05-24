#ifndef TOPAPPS_H
#define TOPAPPS_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QStandardItemModel>
#include <QtCore/QTranslator>
#ifdef QT_DEBUG
#include <QtCore/QDebug>
#endif
#ifdef Q_OS_WIN
#include <os_win.h>
#endif
#include "../global.h"

class TopApps final : public QTabWidget
{
    Q_OBJECT
public:
    TopApps();
    ~TopApps();

private:
    class CStyledItemDelegateEx final : public QStyledItemDelegate
    {
        //Q_OBJECT
    public:
        explicit CStyledItemDelegateEx(TopApps *const parent);
        ~CStyledItemDelegateEx() = default;

    private:
        virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override final;
        const QPen penLight;
        const QPen penDark;
    };

    typedef QPair<qint64, qint64> TPair64;
    enum
    {
        eTypeNull,
        eTypeMinutes,
        eTypeHours,
        eTypeDays,
        eTypeMonths,
        eTypeFromUtc,
        eTypeFromLocal,
        eRoleUnixTime = Qt::UserRole + 1,
        eRoleType,
        eRoleRawValue
    };

    static constexpr const qint64 iMinusSecsPerMonth = -60LL*60*24*(400*365+400/4-3)/(400*12);
    QStandardItemModel *const sitemModel;
    QSpinBox *sbNumOfTop;
    QTreeView *trView;
    QComboBox *cbMethod;
    QPlainTextEdit *pteExclusions;
    QPlainTextEdit *pteErrorLog;
    void FGroupMove(const int iStep);
    inline void FErrorLog(const QString &strError)
    {
#ifdef QT_DEBUG
        qDebug() << strError;
#else
        pteErrorLog->appendPlainText(strError);
        this->setCurrentIndex(3);
#endif
    }
    static inline bool FInBlackList(const QString &strSource, const QStringList &slistExclusions)
    {
        for (const QString &it : slistExclusions)
            if (strSource.startsWith(it))
                return true;
        return false;
    }

private slots:
    void slotUpdate();
    void slotAddGroup();
    inline void slotMoveDown()
    {
        FGroupMove(1);
    }
    inline void slotMoveUp()
    {
        FGroupMove(-1);
    }
    void slotDeleteGroup();
    void slotStopJob();
    void slotRunJob();
};

#endif // TOPAPPS_H
