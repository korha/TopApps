#include "topapps.h"

//-------------------------------------------------------------------------------------------------
TopApps::CStyledItemDelegateEx::CStyledItemDelegateEx(TopApps *const parent) : QStyledItemDelegate(parent),
    penLight(QColor(190, 190, 190)),
    penDark(QColor(84, 84, 84))
{

}

//-------------------------------------------------------------------------------------------------
void TopApps::CStyledItemDelegateEx::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    const QRect &rect = option.rect;
    if (index.parent().isValid())        //items
    {
        painter->setPen(penLight);
        painter->drawLine(rect.topLeft(), rect.topRight());
        painter->setPen(penDark);
    }
    else        //groups
    {
        painter->setPen(penDark);
        painter->drawLine(rect.topLeft(), rect.topRight());
    }
    if (index.column() == 0)
        painter->drawLine(rect.topLeft(), rect.bottomLeft());
    painter->drawLine(rect.topRight(), rect.bottomRight());
}

//-------------------------------------------------------------------------------------------------
TopApps::TopApps() : QTabWidget(),
    sitemModel(new QStandardItemModel(this))
{
    //--- Translator ---
    QString strFilePath = qApp->applicationFilePath() + ".qm";
    QTranslator *const translator = new QTranslator(this);
    if (translator->load(strFilePath))
        qApp->installTranslator(translator);
    strFilePath.chop(2);

    //--- Menu ---
    QLabel *const lblTopApps = new QLabel(this->tr("Top:"), this);
    sbNumOfTop = new QSpinBox(this);
    sbNumOfTop->setRange(1, 500);
    sbNumOfTop->setValue(10);
    QPushButton *const pbUpdate = new QPushButton(this->style()->standardIcon(QStyle::SP_BrowserReload), this->tr("&Update"), this);

    QPushButton *const pbAddGroup = new QPushButton(this->style()->standardIcon(QStyle::SP_FileDialogNewFolder), this->tr("&Add Group"), this);
    QPushButton *const pbMoveDown = new QPushButton(this->style()->standardIcon(QStyle::SP_ArrowDown), nullptr, this);
    pbMoveDown->setToolTip(this->tr("Move Group Down"));
    QPushButton *const pbMoveUp = new QPushButton(this->style()->standardIcon(QStyle::SP_ArrowUp), nullptr, this);
    pbMoveUp->setToolTip(this->tr("Move Group Up"));
    QPushButton *const pbDeleteGroup = new QPushButton(this->style()->standardIcon(QStyle::SP_DialogCancelButton), nullptr, this);
    pbDeleteGroup->setToolTip(this->tr("Delete Group"));

    QPushButton *const pbCollapseAll = new QPushButton(QIcon(":/img/collapse.png"), nullptr, this);
    pbCollapseAll->setToolTip(this->tr("Collapse All"));
    QPushButton *const pbExpandAll = new QPushButton(QIcon(":/img/expand.png"), nullptr, this);
    pbExpandAll->setToolTip(this->tr("Expand All"));

    QPushButton *const pbStopJob = new QPushButton(this->style()->standardIcon(QStyle::SP_MessageBoxCritical), this->tr("&Stop"), this);
    pbStopJob->setToolTip(this->tr("Stop Save Statistics"));

    QHBoxLayout *const hblMenu = new QHBoxLayout;
    hblMenu->setSpacing(2);
    hblMenu->addWidget(lblTopApps);
    hblMenu->addWidget(sbNumOfTop);
    hblMenu->addWidget(pbUpdate);
    hblMenu->addStretch();
    hblMenu->addWidget(pbAddGroup);
    hblMenu->addWidget(pbMoveDown);
    hblMenu->addWidget(pbMoveUp);
    hblMenu->addWidget(pbDeleteGroup);
    hblMenu->addStretch();
    hblMenu->addWidget(pbCollapseAll);
    hblMenu->addWidget(pbExpandAll);
    hblMenu->addWidget(pbStopJob);

    //--- Tree view ---
    trView = new QTreeView(this);
#ifdef Q_OS_WIN
    trView->setFont(QFont("Trebuchet MS", 9));
#endif
    trView->setModel(sitemModel);
    trView->setItemDelegate(new CStyledItemDelegateEx(this));
    sitemModel->setHorizontalHeaderLabels(QStringList(this->tr("Application")) << this->tr("Time"));

    //--- Main tab ---
    QWidget *const wgtStatistics = new QWidget(this);
    QVBoxLayout *const vblStatistics = new QVBoxLayout(wgtStatistics);
    vblStatistics->setContentsMargins(5, 5, 5, 5);
    vblStatistics->addLayout(hblMenu);
    vblStatistics->addWidget(trView);

    //--- New job ---
    QLabel *const lblMethods = new QLabel(this->tr("Method:"), this);
    cbMethod = new QComboBox(this);
    cbMethod->addItems(COsSpec::FGetMethods());
    QPushButton *const pbRunJob = new QPushButton(this->style()->standardIcon(QStyle::SP_CommandLink), this->tr("Run"), this);
    QFrame *const frmJob = new QFrame(this);
    frmJob->setFrameStyle(QFrame::StyledPanel);
    QVBoxLayout *const vblJob = new QVBoxLayout(frmJob);
    vblJob->addWidget(lblMethods, 0, Qt::AlignHCenter);
    vblJob->addWidget(cbMethod);
    vblJob->addWidget(pbRunJob, 0, Qt::AlignHCenter);
    QWidget *const wgtNewJob = new QWidget(this);
    (new QVBoxLayout(wgtNewJob))->addWidget(frmJob, 0, Qt::AlignCenter);

    //--- Exclusions & Error log ---
    pteExclusions = new QPlainTextEdit(this);
    pteErrorLog = new QPlainTextEdit(nullptr);
    pteErrorLog->setReadOnly(true);

    //--- Main ---
    this->addTab(wgtStatistics, this->tr("Statistics"));
    this->addTab(wgtNewJob, this->tr("New Job"));
    this->addTab(pteExclusions, this->tr("Exclusions"));
    this->addTab(pteErrorLog, this->tr("Errors Log"));

    QWidget *const pStackWgt = this->findChild<QWidget*>("qt_tabwidget_stackedwidget", Qt::FindDirectChildrenOnly);
    Q_ASSERT(pStackWgt);
    if (pStackWgt)
        pStackWgt->setAutoFillBackground(true);        //fill background hack

    this->setMinimumSize(this->minimumSizeHint());        //prevent collapse

    this->connect(pbUpdate, QPushButton::clicked, this, slotUpdate);
    this->connect(pbAddGroup, QPushButton::clicked, this, slotAddGroup);
    this->connect(pbMoveDown, QPushButton::clicked, this, slotMoveDown);
    this->connect(pbMoveUp, QPushButton::clicked, this, slotMoveUp);
    this->connect(pbDeleteGroup, QPushButton::clicked, this, slotDeleteGroup);
    this->connect(pbCollapseAll, QPushButton::clicked, trView, QTreeView::collapseAll);
    this->connect(pbExpandAll, QPushButton::clicked, trView, QTreeView::expandAll);
    this->connect(pbStopJob, QPushButton::clicked, this, slotStopJob);
    this->connect(pbRunJob, QPushButton::clicked, this, slotRunJob);

    //--- Restore exclusions ---
    strFilePath += "exc";
    QFile file(strFilePath);
    strFilePath.chop(3);
    if (file.open(QFile::ReadOnly))
    {
        const QByteArray baData = file.readAll();
        const bool bOk = !file.error();
        file.close();
        if (bOk && baData.size() > 0)
        {
            Q_ASSERT(reinterpret_cast<size_t>(baData.constData())%sizeof(QChar) == 0);
            pteExclusions->setPlainText(QString::fromRawData(pointer_cast<const QChar*>(baData.constData()), baData.size()/sizeof(QChar)));
        }
    }

    //--- Restore layout ---
    strFilePath += "lay";
    file.setFileName(strFilePath);
    strFilePath.chop(3);
    if (file.open(QFile::ReadOnly))
    {
        const QByteArray baData = file.readAll();
        const bool bOk = !file.error();
        file.close();
        if (bOk && baData.size() >= static_cast<int>(sizeof(int)*2) + 4)
        {
            const char *const cConstData = baData.constData();
            if (this->restoreGeometry(QByteArray::fromRawData(cConstData + sizeof(int)*2, baData.size() - sizeof(int)*2)))
            {
                int iBuf[2];
                memcpy(iBuf, cConstData, sizeof(iBuf));
                trView->header()->resizeSection(0, iBuf[0]);
                sbNumOfTop->setValue(iBuf[1]);
            }
        }
    }

    //--- Restore groups ---
    strFilePath += "grp";
    file.setFileName(strFilePath);
    //strFilePath.chop(3);
    if (file.open(QFile::ReadOnly))
    {
        const QByteArray baData = file.readAll();
        const bool bOk = !file.error();
        file.close();
        if (bOk && baData.size() > 0 && baData.size() <= static_cast<int>(100*sizeof(qint64)*2) && baData.size()%(sizeof(qint64)*2) == 0)
        {
            const qint64 *pIt = pointer_cast<const qint64*>(baData.constData());
            Q_ASSERT(reinterpret_cast<size_t>(pIt)%sizeof(qint64) == 0);
            const qint64 *const pEnd = pointer_cast<const qint64 *>(pointer_cast<const char*>(pIt) + baData.size());
            while (pIt < pEnd)
            {
                qint64 iType = *pIt++;
                const qint64 iRawValue = *pIt++;
                qint64 iUnixTime = iRawValue;
                QString strGroupName;
                switch (iType)
                {
                case eTypeMinutes:
                    if (iUnixTime >= 10 && iUnixTime <= 24*60)
                    {
                        strGroupName = this->tr("Last %n minute(s)", nullptr, iUnixTime);
                        iUnixTime *= -60;        //negative value is "last-time"
                    }
                    else
                        iType = eTypeNull;
                    break;
                case eTypeHours:
                    if (iUnixTime >= 1 && iUnixTime <= 31*24)
                    {
                        strGroupName = this->tr("Last %n hour(s)", nullptr, iUnixTime);
                        iUnixTime *= -60*60;
                    }
                    else
                        iType = eTypeNull;
                    break;
                case eTypeDays:
                    if (iUnixTime >= 1 && iUnixTime <= 366)
                    {
                        strGroupName = this->tr("Last %n day(s)", nullptr, iUnixTime);
                        iUnixTime *= -60*60*24;
                    }
                    else
                        iType = eTypeNull;
                    break;
                case eTypeMonths:
                    if (iUnixTime >= 0 && iUnixTime <= 120)
                    {
                        strGroupName = this->tr("Last %n month(s)", nullptr, iUnixTime);
                        iUnixTime *= iMinusSecsPerMonth;
                    }
                    else
                        iType = eTypeNull;
                    break;
                case eTypeFromUtc:
                    if (iUnixTime > 0)
                        strGroupName = this->tr("From:") + QDateTime::fromSecsSinceEpoch(iUnixTime, Qt::UTC).toString(" yyyy-MM-dd h:mm") + " UTC";
                    else
                        iType = eTypeNull;
                    break;
                case eTypeFromLocal:
                    if (iUnixTime > 0)
                        strGroupName = this->tr("From:") + QDateTime::fromSecsSinceEpoch(iUnixTime, Qt::UTC).toString(" yyyy-MM-dd h:mm");
                    else
                        iType = eTypeNull;
                    break;
                default: iType = eTypeNull;
                }

                if (iType != eTypeNull)
                {
                    QStandardItem *const sitemGroupName = new QStandardItem(this->style()->standardIcon(QStyle::SP_DialogYesButton), strGroupName);
                    sitemGroupName->setEditable(false);
                    sitemGroupName->setData(iUnixTime, eRoleUnixTime);
                    sitemGroupName->setData(iType, eRoleType);
                    sitemGroupName->setData(iRawValue, eRoleRawValue);
                    QStandardItem *const sitemGroupTime = new QStandardItem;
                    sitemGroupTime->setEditable(false);
                    QList<QStandardItem*> listGroup;
                    listGroup.append(sitemGroupName);
                    listGroup.append(sitemGroupTime);
                    sitemModel->appendRow(listGroup);
                }
            }
        }
    }

    //--- Misc ---
    slotUpdate();
    trView->expandAll();
}

//-------------------------------------------------------------------------------------------------
TopApps::~TopApps()
{
    //read-only flag -> lock settings

    //--- Save exclusions ---
    QString strFilePath = qApp->applicationFilePath() + ".exc";
    QFile file(strFilePath);
    strFilePath.chop(3);
    if (file.open(QFile::WriteOnly))
    {
        const QString strExclusions = pteExclusions->toPlainText();
        file.write(QByteArray::fromRawData(pointer_cast<const char*>(strExclusions.constData()), strExclusions.size()*sizeof(QChar)));
        file.close();
    }

    //--- Save layout ---
    strFilePath += "lay";
    file.setFileName(strFilePath);
    strFilePath.chop(3);
    if (file.open(QFile::WriteOnly))
    {
        const int iBuf[] {trView->header()->sectionSize(0), sbNumOfTop->value()};
        file.write(QByteArray::fromRawData(pointer_cast<const char*>(&iBuf), sizeof(iBuf)));
        file.write(this->saveGeometry());
        file.close();
    }

    //--- Save groups ---
    const int iRowCount = qMin(sitemModel->rowCount(), 100);
    if (iRowCount > 0)
    {
        QByteArray baData;
        baData.reserve(iRowCount*sizeof(qint64)*2);
        for (int i = 0; i < iRowCount; ++i)
        {
            const QModelIndex modInd = sitemModel->index(i, 0);
            const qint64 iBuf[] = {modInd.data(eRoleType).toLongLong(), modInd.data(eRoleRawValue).toLongLong()};
            baData.append(pointer_cast<const char*>(iBuf), sizeof(iBuf));
        }

        strFilePath += "grp";
        file.setFileName(strFilePath);
        //strFilePath.chop(3);
        if (file.open(QFile::WriteOnly))
        {
            file.write(baData);
            //file.close();
        }
    }
}

//-------------------------------------------------------------------------------------------------
void TopApps::slotUpdate()
{
    this->setWindowIcon(QIcon(COsSpec::FIsSaveStatEnabled() ? ":/img/on.png" : ":/img/off.png"));
    const int iModelRowCount = sitemModel->rowCount();
    if (iModelRowCount < 1)
    {
        slotAddGroup();
        return;
    }

    //--- Clean tree view ---
    QStandardItem *const sitemRoot = sitemModel->invisibleRootItem();
    for (int i = sitemRoot->rowCount()-1; i >= 0; --i)
    {
        QStandardItem *const sitem = sitemRoot->child(i);
        for (int j = sitem->rowCount()-1; j >= 0; --j)
            sitem->removeRow(j);
    }

    //--- Load statistics ---
    COsSpec::CMutex cMutex;        //compatibility with the OS mutex
    if (!cMutex.FWait())
    {
        FErrorLog("Mutex failed");
        return;
    }

    QString strFilePath = qApp->applicationDirPath() + "/index.dat";
    QFile file(strFilePath);
    if (!file.open(QFile::ReadOnly))
    {
        FErrorLog("index.dat open failed");
        setCurrentIndex(1);
        return;
    }

    const QByteArray baIndexData = file.readAll();
    const bool bOk = !file.error();
    file.close();
    int iSizeAll = baIndexData.size();
    if (!(bOk && iSizeAll > 0 && iSizeAll%(COsSpec::iAppPathSize*sizeof(wchar_t)) == 0))
    {
        FErrorLog("index.dat is invalid");
        return;
    }

    strFilePath.chop(9);        //"index.dat"
    strFilePath += "data/";
    strFilePath.resize(strFilePath.size() + sizeof(quint32)*2/*hex*/);

    //--- Parse statistics ---
    const wchar_t *wIt = pointer_cast<const wchar_t*>(baIndexData.constData());
    Q_ASSERT(reinterpret_cast<size_t>(wIt)%sizeof(wchar_t) == 0);
    iSizeAll /= COsSpec::iAppPathSize*sizeof(wchar_t);        //filesize to number of apps
    QVector<QString> vectAppPaths(iSizeAll);
    QVector<QPixmap> vectPixmaps(iSizeAll);
    QVector<QVector<TPair64>> vectPeriodsAll(iSizeAll);
    for (int i = 0; i < iSizeAll; ++i)
    {
        if (wIt[COsSpec::iAppPathSize-1] == L'\0')        //check to out of bounds
        {
            strFilePath.chop(sizeof(quint32)*2);
            strFilePath += QString("%1").arg(i, sizeof(quint32)*2, 16, QChar('0'));
            file.setFileName(strFilePath);
            if (file.open(QFile::ReadOnly))
            {
                const QByteArray baTimeData = file.readAll();
                const bool bOk = !file.error();
                file.close();
                int iSize = baTimeData.size();
                if (bOk && iSize%(sizeof(qint32)*2) == 0)
                {
                    const QString strAppPath = QString::fromWCharArray(wIt);
                    vectAppPaths[i] = strAppPath;
                    vectPixmaps[i] = COsSpec::FGetPixmap(strAppPath);

                    //add periods
                    const qint64 *pIt = pointer_cast<const qint64*>(baTimeData.constData());
                    Q_ASSERT(reinterpret_cast<size_t>(pIt)%sizeof(qint64) == 0);
                    iSize /= sizeof(qint64)*2;        //filesize to number of periods
                    QVector<TPair64> &vectPeriods = vectPeriodsAll[i];
                    vectPeriods.resize(iSize);
                    for (int j = 0; j < iSize; ++j)
                    {
                        const qint64 iFirst = *pIt++;
                        vectPeriods[j] = qMakePair(iFirst, *pIt++);
                    }
                }
                else
                    FErrorLog("Datafile is invalid #" + QString::number(i));
            }
            else
                FErrorLog("Datafile open failed #" + QString::number(i));
        }
        else
            FErrorLog("Invalid app path #" + QString::number(i));
        wIt += COsSpec::iAppPathSize;
    }

    //--- Add statistics from running apps ---
    const QVector<QPair<QString, qint64>> vectRunningApps = COsSpec::FGetStatFromRunningApps();        //moved

    cMutex.FRelease();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    const qint32 iUnixTimeCurrent = QDateTime::currentSecsSinceEpoch();
#else
    const qint32 iUnixTimeCurrent = QDateTime::currentMSecsSinceEpoch()/1000;
#endif
    for (int i = vectRunningApps.size()-1; i >= 0; --i)
    {
        int j = vectAppPaths.size()-1;
        for (; j >= 0; --j)
            if (vectRunningApps.at(i).first == vectAppPaths.at(j))        //exists
            {
                vectPeriodsAll[j].append(qMakePair(vectRunningApps.at(i).second, iUnixTimeCurrent));
                break;
            }
        if (j < 0)        //new
        {
            vectAppPaths.append(vectRunningApps.at(i).first);
            vectPixmaps.append(COsSpec::FGetPixmap(vectRunningApps.at(i).first));
            vectPeriodsAll.append(QVector<TPair64>(1, qMakePair(vectRunningApps.at(i).second, iUnixTimeCurrent)));
        }
    }
    iSizeAll = vectAppPaths.size();

    //--- Clean Up ---
    for (int i = iSizeAll-1; i >= 0; --i)
    {
        QVector<TPair64> &vect = vectPeriodsAll[i];

        //0-sec periods
        for (int j = vect.size()-1; j >= 0; --j)
            if (vect.at(j).first == vect.at(j).second)
                vect.remove(j);

        //corruptions
        for (int j = vect.size()-1; j >= 0; --j)
            if (vect.at(j).first <= 0 || vect.at(j).second <= 0 || vect.at(j).first > vect.at(j).second)
            {
                vect.clear();
                break;
            }
        for (int j = vect.size()-1; j > 0; --j)
            if (vect.at(j).first < vect.at(j-1).first)
            {
                vect.clear();
                break;
            }

        //squeeze (multiple copies running simultaneously):
        //A-D;B-C -> A-D
        for (int j = vect.size()-1; j > 0; --j)
            if (vect.at(j).second <= vect.at(j-1).second)
            {
                vect.remove(j);
                if (j < vect.size())
                    ++j;
            }
        //A-C;B-D -> A->D
        for (int j = vect.size()-1; j > 0; --j)
            if (vect.at(j).first <= vect.at(j-1).second)
            {
                vect[j-1].second = vect.at(j).second;
                vect.remove(j);
                if (j < vect.size())
                    ++j;
            }
    }

    //--- Fill tree view --
    const QStringList slistExclusions = pteExclusions->toPlainText().split('\n', QString::SkipEmptyParts);
    const int iNumOfTopAll = sbNumOfTop->value();
    trView->setUpdatesEnabled(false);
    for (int i = 0; i < iModelRowCount; ++i)        //groups
    {
        qint64 iUnixTimeFrom = sitemModel->index(i, 0).data(eRoleUnixTime).toLongLong();
        if (iUnixTimeFrom < 0)        //negative value is "last-time"
            iUnixTimeFrom += iUnixTimeCurrent;

        QVector<TPair64> vectTotalTimes(iSizeAll);
        for (int j = 0; j < iSizeAll; ++j)        //apps per groups
        {
            qint64 iLength = 0;
            const QVector<TPair64> &vect = vectPeriodsAll.at(j);
            const int iVecSize = vect.size();
            for (int k = 0; k < iVecSize; ++k)
                if (iUnixTimeFrom <= vect.at(k).first)
                {
                    if (k > 0 && vect.at(k-1).second > iUnixTimeFrom)
                        iLength = vect.at(k-1).second-iUnixTimeFrom;
                    for (; k < iVecSize; ++k)
                        iLength += vect.at(k).second-vect.at(k).first;
                }
            if (!iLength && iVecSize && vect.at(iVecSize-1).second > iUnixTimeFrom)
                iLength = vect.at(iVecSize-1).second-iUnixTimeFrom;
            vectTotalTimes[j] = qMakePair(j, iLength);
        }

        std::sort(vectTotalTimes.begin(), vectTotalTimes.end(), [](TPair64 i, TPair64 j){return (i.second > j.second);});

        QStandardItem *const sitemGroup = sitemRoot->child(i);
        for (int j = 0, iTotal = vectTotalTimes.size(), iNumOfTop = iNumOfTopAll; j < iNumOfTop && j < iTotal; ++j)
        {
            const int iOriginalNum = vectTotalTimes.at(j).first;
            const QString &strAppPath = vectAppPaths.at(iOriginalNum);
            if (FInBlackList(strAppPath, slistExclusions))
                ++iNumOfTop;
            else
            {
                qint64 iLength = vectTotalTimes.at(j).second;
                QString strHumanTime;
                if (iLength >= 60)
                {
                    qint64 iTemp = iLength/(60*60*24);
                    if (iTemp)
                    {
                        strHumanTime = QString::number(iTemp) + this->tr("d", "day") + " ";
                        iLength -= iTemp*(60*60*24);
                    }
                    iTemp = iLength/(60*60);
                    if (iTemp)
                    {
                        strHumanTime += QString::number(iTemp) + this->tr("h", "hour") + " ";
                        iLength -= iTemp*(60*60);
                    }
                    strHumanTime += QString::number(iLength/60);
                }
                else
                    strHumanTime = "<1";
                strHumanTime += this->tr("m", "minute");

                QStandardItem *const sitemAppName = new QStandardItem(vectPixmaps.at(iOriginalNum), strAppPath);
                sitemAppName->setEditable(false);
                QStandardItem *const sitemGroupTime = new QStandardItem(strHumanTime);
                sitemGroupTime->setEditable(false);
                QList<QStandardItem*> listGroup;
                listGroup.append(sitemAppName);
                listGroup.append(sitemGroupTime);
                sitemGroup->appendRow(listGroup);
            }
        }
    }
    trView->setUpdatesEnabled(true);
}

//-------------------------------------------------------------------------------------------------
void TopApps::slotAddGroup()
{
    //--- Prepare dialog ---
    QDialog dialog(nullptr, Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(this->tr("Add Group"));
    QWidget *const parent = &dialog;

    QLabel *const lblLast = new QLabel(this->tr("Last:"), parent);

    QRadioButton *const rbMinutes = new QRadioButton(parent);
    rbMinutes->setChecked(true);
    QSpinBox *const sbMinutes = new QSpinBox(parent);
    sbMinutes->setRange(10, 24*60);
    QLabel *const lblMinutes = new QLabel(this->tr("&minutes"), parent);
    lblMinutes->setBuddy(rbMinutes);

    QRadioButton *const rbHours = new QRadioButton(parent);
    QSpinBox *const sbHours = new QSpinBox(parent);
    sbHours->setRange(1, 31*24);
    QLabel *const lblHours = new QLabel(this->tr("&hours"), parent);
    lblHours->setBuddy(rbHours);

    QRadioButton *const rbDays = new QRadioButton(parent);
    QSpinBox *const sbDays = new QSpinBox(parent);
    sbDays->setRange(1, 366);
    QLabel *const lblDays = new QLabel(this->tr("&days"), parent);
    lblDays->setBuddy(rbDays);

    QRadioButton *const rbMonths = new QRadioButton(parent);
    QSpinBox *const sbMonths = new QSpinBox(parent);
    sbMonths->setRange(1, 120);
    QLabel *const lblMonths = new QLabel(this->tr("m&onths"), parent);
    lblMonths->setBuddy(rbMonths);

    QLabel *const lblFrom = new QLabel(this->tr("&From:"), parent);

    QDateTimeEdit *const dtEdit = new QDateTimeEdit(parent);
    dtEdit->setDateRange(QDate(2000, 1, 1), QDate(2100, 1, 1));
    dtEdit->setDateTime(QDateTime(QDateTime::currentDateTime().date()));
    lblFrom->setBuddy(dtEdit);

    QRadioButton *const rbFromUtc = new QRadioButton("&UTC", parent);
    QRadioButton *const rbFromLocal = new QRadioButton(this->tr("&Local"), parent);
    QHBoxLayout *const hblFrom = new QHBoxLayout;
    hblFrom->addWidget(rbFromUtc);
    hblFrom->addWidget(rbFromLocal);

    QDialogButtonBox *const dialogBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, parent);

    QGridLayout *const gridMain = new QGridLayout(parent);
    gridMain->addWidget(lblLast, 0, 0, 1, 3, Qt::AlignHCenter);
    gridMain->addWidget(rbMinutes, 1, 0);
    gridMain->addWidget(sbMinutes, 1, 1);
    gridMain->addWidget(lblMinutes, 1, 2);
    gridMain->addWidget(rbHours, 2, 0);
    gridMain->addWidget(sbHours, 2, 1);
    gridMain->addWidget(lblHours, 2, 2);
    gridMain->addWidget(rbDays, 3, 0);
    gridMain->addWidget(sbDays, 3, 1);
    gridMain->addWidget(lblDays, 3, 2);
    gridMain->addWidget(rbMonths, 4, 0);
    gridMain->addWidget(sbMonths, 4, 1);
    gridMain->addWidget(lblMonths, 4, 2);
    gridMain->addWidget(lblFrom, 5, 0, 1, 3, Qt::AlignHCenter);
    gridMain->addWidget(dtEdit, 6, 0, 1, 3, Qt::AlignHCenter);
    gridMain->addLayout(hblFrom, 7, 0, 1, 3, Qt::AlignHCenter);
    gridMain->addWidget(dialogBox, 8, 0, 1, 3, Qt::AlignHCenter);

    dialog.setFixedSize(dialog.minimumSizeHint());

    QObject::connect(dialogBox, QDialogButtonBox::accepted, &dialog, QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        //--- Get result ---
        qint64 iType;
        qint64 iUnixTime;        //negative value is "last-time"
        qint64 iRawValue;
        QString strGroupName;
        if (rbMinutes->isChecked())
        {
            iType = eTypeMinutes;
            iUnixTime = sbMinutes->value();
            strGroupName = this->tr("Last %n minute(s)", nullptr, iUnixTime);
            iRawValue = iUnixTime;
            iUnixTime *= -60;
        }
        else if (rbHours->isChecked())
        {
            iType = eTypeHours;
            iUnixTime = sbHours->value();
            strGroupName = this->tr("Last %n hour(s)", nullptr, iUnixTime);
            iRawValue = iUnixTime;
            iUnixTime *= -60*60;
        }
        else if (rbDays->isChecked())
        {
            iType = eTypeDays;
            iUnixTime = sbDays->value();
            strGroupName = this->tr("Last %n day(s)", nullptr, iUnixTime);
            iRawValue = iUnixTime;
            iUnixTime *= -60*60*24;
        }
        else if (rbMonths->isChecked())
        {
            iType = eTypeMonths;
            iUnixTime = sbMonths->value();
            strGroupName = this->tr("Last %n month(s)", nullptr, iUnixTime);
            iRawValue = iUnixTime;
            iUnixTime *= iMinusSecsPerMonth;
        }
        else if (rbFromUtc->isChecked())
        {
            iType = eTypeFromUtc;
            const QDateTime dt = dtEdit->dateTime();
            strGroupName = this->tr("From:") + dt.toString(" yyyy-MM-dd h:mm") + " UTC";
            iUnixTime = dt.toUTC().toSecsSinceEpoch();
            iRawValue = iUnixTime;
        }
        else
        {
            Q_ASSERT(rbFromLocal->isChecked());
            iType = eTypeFromLocal;
            const QDateTime dt = dtEdit->dateTime();
            strGroupName = this->tr("From:") + dt.toString(" yyyy-MM-dd h:mm");
            iUnixTime = dt.toLocalTime().toSecsSinceEpoch();
            iRawValue = iUnixTime;
        }

        //--- Fill tree view ---
        QStandardItem *const sitemGroupName = new QStandardItem(this->style()->standardIcon(QStyle::SP_DialogYesButton), strGroupName);
        sitemGroupName->setEditable(false);
        sitemGroupName->setData(iUnixTime, eRoleUnixTime);
        sitemGroupName->setData(iType, eRoleType);
        sitemGroupName->setData(iRawValue, eRoleRawValue);
        QStandardItem *const sitemGroupTime = new QStandardItem;
        sitemGroupTime->setEditable(false);
        QList<QStandardItem*> listGroup;
        listGroup.append(sitemGroupName);
        listGroup.append(sitemGroupTime);
        sitemModel->appendRow(listGroup);
        trView->setFocus();
    }
}

//-------------------------------------------------------------------------------------------------
void TopApps::FGroupMove(const int iStep)
{
    Q_ASSERT(iStep == 1 || iStep == -1);
    QItemSelectionModel *const itemSelModel = trView->selectionModel();
    const QModelIndexList modIndList = itemSelModel->selectedIndexes();
    if (!modIndList.isEmpty())
    {
        const QModelIndex &modInd = modIndList.first();
        if (!modInd.parent().isValid())        //group
        {
            const int iRow = modInd.row();
            if (iStep > 0 ? (iRow < sitemModel->rowCount()-1) : (iRow > 0))
            {
                const bool bIsExpanded = trView->isExpanded(modInd);
                const QList<QStandardItem*> listTakes = sitemModel->takeRow(iRow);
                sitemModel->insertRow(iRow+iStep, listTakes);
                const QModelIndex modIndHost = sitemModel->indexFromItem(listTakes.first());
                if (bIsExpanded)
                    trView->expand(modIndHost);
                itemSelModel->select(QItemSelection(modIndHost, sitemModel->indexFromItem(listTakes.at(1))), QItemSelectionModel::ClearAndSelect);
                trView->scrollTo(modIndHost);
                trView->setFocus();
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
void TopApps::slotDeleteGroup()
{
    const QModelIndexList modIndList = trView->selectionModel()->selectedIndexes();
    if (!modIndList.isEmpty())
    {
        const QModelIndex &modInd = modIndList.first();
        sitemModel->removeRow(modInd.row(), modInd.parent());
        trView->setFocus();
    }
}

//-------------------------------------------------------------------------------------------------
void TopApps::slotStopJob()
{
    COsSpec::FStopJob();
    const bool bIsStatEnabled = COsSpec::FIsSaveStatEnabled();
    this->setWindowIcon(QIcon(bIsStatEnabled ? ":/img/on.png" : ":/img/off.png"));
    if (bIsStatEnabled)
        QMessageBox::warning(this, nullptr, this->tr("Close all apps or restart the OS to avoid confusions"));
}

//-------------------------------------------------------------------------------------------------
void TopApps::slotRunJob()
{
    COsSpec::FStopJob();
    if (COsSpec::FIsSaveStatEnabled())
        QMessageBox::warning(this, nullptr, this->tr("Previous job is activated"));
    else
    {
        const char *const cError = COsSpec::FRunJob(cbMethod->currentIndex());
        if (cError)
            QMessageBox::warning(this, nullptr, cError);
        else
            QMessageBox::information(this, nullptr, this->tr("Success. Restart apps or the OS for better results"));
    }
}
