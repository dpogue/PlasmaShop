/* This file is part of PlasmaShop.
 *
 * PlasmaShop is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PlasmaShop is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PlasmaShop.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QDrawInterface.h"

#include <PRP/Geometry/plDrawableSpans.h>
#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>
#include <QMenu>
#include <QDialogButtonBox>
#include "../../QKeyDialog.h"
#include "../../Main.h"

/* QDrawableList */
QDrawableList::QDrawableList(plKey container, QWidget* parent)
             : QKeyList(container, parent)
{
    setColumnCount(2);
    setUniformRowHeights(true);
    setRootIsDecorated(false);
    headerItem()->setText(0, tr("Key"));
    headerItem()->setText(1, tr("Drawable"));
}

void QDrawableList::addKey(plKey key, int dkey)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(this,
        QStringList() << QString("%1").arg(dkey) << ~key->getName());
    item->setIcon(1, pqGetTypeIcon(key->getType()));
    fKeys << key;
    fDrawKeys << dkey;
}

void QDrawableList::contextMenuEvent(QContextMenuEvent* evt)
{
    QMenu menu(this);
    QAction* addObjItem = menu.addAction(tr("Add Object"));
    QAction* delObjItem = menu.addAction(tr("Remove Object"));
    menu.addSeparator();
    QAction* openMatItem = menu.addAction(tr("Open Materials"));
    QAction* openIcicle = menu.addAction(tr("Open Icicles"));

    if (currentItem() == NULL)
        delObjItem->setEnabled(false);

    QAction* sel = menu.exec(evt->globalPos());
    if (sel == addObjItem) {
        QFindDrawKeyDialog dlg(this);
        dlg.init(PrpShopMain::ResManager(), fContainer->getLocation());
        if (dlg.exec() == QDialog::Accepted)
            addKey(dlg.selection(), dlg.drawKey());
    } else if (sel == delObjItem) {
        delItem(indexOfTopLevelItem(currentItem()));
    } else if (sel == openMatItem) {
        plResManager* mgr = PrpShopMain::ResManager();
        int idx = indexOfTopLevelItem(currentItem());
        plDrawableSpans* dspans = plDrawableSpans::Convert(mgr->getObject(fKeys[idx]));
        plDISpanIndex dis = dspans->getDIIndex(fDrawKeys[idx]);

        std::vector<plKey> mats = dspans->getMaterials();
        for (size_t i = 0; i < dis.fIndices.size(); i++) {
            plIcicle* ice = dspans->getIcicle(dis.fIndices[i]);
            hsKeyedObject* mat = mgr->getObject(mats[ice->getMaterialIdx()]);
            PrpShopMain::Instance()->editCreatable(mat);
        }
    } else if (sel == openIcicle) {
        plResManager* mgr = PrpShopMain::ResManager();
        int idx = indexOfTopLevelItem(currentItem());
        plDrawableSpans* dspans = plDrawableSpans::Convert(mgr->getObject(fKeys[idx]));
        plDISpanIndex dis = dspans->getDIIndex(fDrawKeys[idx]);

        std::vector<plKey> mats = dspans->getMaterials();
        for (size_t i = 0; i < dis.fIndices.size(); i++) {
            plIcicle* ice = dspans->getIcicle(dis.fIndices[i]);
            plKey mat = mats[ice->getMaterialIdx()];

            qtIcicleData* icicle = new qtIcicleData();
            icicle->fIcicle = ice;
            icicle->fMaterial = mat;
            icicle->fDIOwner = fContainer;

            PrpShopMain::Instance()->editCreatable((plCreatable*)(void*)(icicle), kIcicle);
        }
    }
}


/* QFindDrawKeyDialog */
QFindDrawKeyDialog::QFindDrawKeyDialog(QWidget* parent)
                  : QDialog(parent)
{
    setWindowTitle(tr("Add Drawable..."));

    fKeyBox = new QComboBox(this);
    fDrawKey = new QSpinBox(this);
    fDrawKey->setRange(-0x80000000, 0x7FFFFFFF);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(Qt::Horizontal, this);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel |
                                  QDialogButtonBox::Ok);

    QGridLayout* layout = new QGridLayout(this);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(8);
    layout->addWidget(new QLabel(tr("Drawable:"), this), 0, 0);
    layout->addWidget(fKeyBox, 0, 1);
    layout->addWidget(new QLabel(tr("DI Key:"), this), 1, 0);
    layout->addWidget(fDrawKey, 1, 1);
    layout->addWidget(buttonBox, 2, 0, 1, 2);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void QFindDrawKeyDialog::init(plResManager* mgr, const plLocation& loc)
{
    fResMgr = mgr;
    fLocation = loc;

    fKeyBox->clear();
    fKeys = fResMgr->getKeys(fLocation, kDrawableSpans);
    for (size_t i=0; i<fKeys.size(); i++)
        fKeyBox->addItem(~fKeys[i]->getName());
    fDrawKey->setValue(0);
}


/* QDrawInterface */
QDrawInterface::QDrawInterface(plCreatable* pCre, QWidget* parent)
              : QCreatable(pCre, kDrawInterface, parent)
{
    plDrawInterface* intf = plDrawInterface::Convert(fCreatable);

    fOwnerLink = new QCreatableLink(this);
    fOwnerLink->setText(tr("Owner Object"));
    fOwnerLink->setKey(intf->getOwner());

    fSynchObjLink = new QCreatableLink(this, false);
    fSynchObjLink->setText(tr("Synch Flags"));
    fSynchObjLink->setCreatable(intf);
    fSynchObjLink->setForceType(kSynchedObject);

    QGroupBox* grpProps = new QGroupBox(tr("Properties"), this);
    QGridLayout* layProps = new QGridLayout(grpProps);
    layProps->setVerticalSpacing(0);
    fDisabled = new QCheckBox(tr("Disable"), grpProps);
    layProps->addWidget(fDisabled, 0, 0);
    fDisabled->setChecked(intf->getProperty(plDrawInterface::kDisable));

    QTabWidget* childTab = new QTabWidget(this);

    fDrawKeys = new QDrawableList(intf->getKey(), childTab);
    for (size_t i=0; i<intf->getNumDrawables(); i++)
        fDrawKeys->addKey(intf->getDrawable(i), intf->getDrawableKey(i));

    fRegions = new QKeyList(intf->getKey(), childTab);
    for (size_t i=0; i<intf->getRegions().size(); i++)
        fRegions->addKey(intf->getRegions()[i]);

    childTab->addTab(fDrawKeys, tr("Drawables"));
    childTab->addTab(fRegions, tr("Regions"));

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(fOwnerLink, 0, 0);
    layout->addWidget(fSynchObjLink, 1, 0);
    layout->addWidget(grpProps, 2, 0);
    layout->addWidget(childTab, 3, 0);
    fDrawKeys->adjustColumns();
    fRegions->adjustColumns();

    connect(fOwnerLink, SIGNAL(addObject()), this, SLOT(setOwner()));
    connect(fOwnerLink, SIGNAL(delObject()), this, SLOT(unsetOwner()));
}

void QDrawInterface::saveDamage()
{
    plDrawInterface* intf = plDrawInterface::Convert(fCreatable);
    intf->setProperty(plDrawInterface::kDisable, fDisabled->isChecked());

    intf->clearDrawables();
    QList<plKey> drawables = fDrawKeys->keys();
    QList<int> dkeys = fDrawKeys->drawKeys();
    while (!drawables.isEmpty())
        intf->addDrawable(drawables.takeFirst(), dkeys.takeFirst());

    intf->clearRegions();
    QList<plKey> regions = fRegions->keys();
    while (!regions.isEmpty())
        intf->addRegion(regions.takeFirst());
}

void QDrawInterface::setOwner()
{
    plDrawInterface* intf = plDrawInterface::Convert(fCreatable);
    QFindKeyDialog dlg(this);
    if (intf->getOwner().Exists())
        dlg.init(PrpShopMain::ResManager(), intf->getOwner());
    else
        dlg.init(PrpShopMain::ResManager(), intf->getKey()->getLocation(), kSceneObject);
    if (dlg.exec() == QDialog::Accepted) {
        intf->setOwner(dlg.selection());
        fOwnerLink->setKey(intf->getOwner());
    }
}

void QDrawInterface::unsetOwner()
{
    plDrawInterface* intf = plDrawInterface::Convert(fCreatable);
    intf->setOwner(plKey());
    fOwnerLink->setCreatable(NULL);
}


/* QIcicle */
QIcicle::QIcicle(plCreatable* pCre, QWidget* parent)
              : QCreatable(pCre, kIcicle, parent)
{
    // NOTE: fCreatable is NOT a plCreatable!
    this->fData = (qtIcicleData*)(void*)(fCreatable);

    QGroupBox* grpFlags = new QGroupBox(tr("Flags"), this);
    fFlags[kPropNoDraw] = new QCheckBox(tr("No Draw"), grpFlags);
    fFlags[kPropNoShadowCast] = new QCheckBox(tr("No Shadow Cast"), grpFlags);
    fFlags[kPropFacesSortable] = new QCheckBox(tr("Faces Sortable"), grpFlags);
    fFlags[kPropVolatile] = new QCheckBox(tr("Volatile"), grpFlags);
    fFlags[kWaterHeight] = new QCheckBox(tr("Water Height"), grpFlags);
    fFlags[kPropRunTimeLight] = new QCheckBox(tr("Run-Time Light"), grpFlags);
    fFlags[kPropReverseSort] = new QCheckBox(tr("Reverse Sort"), grpFlags);
    fFlags[kPropHasPermaLights] = new QCheckBox(tr("PermaLights"), grpFlags);
    fFlags[kPropHasPermaProjs] = new QCheckBox(tr("PermaProjs"), grpFlags);
    fFlags[kPropMatHasSpecular] = new QCheckBox(tr("Has Specular"), grpFlags);
    fFlags[kPropProjAsVtx] = new QCheckBox(tr("ProjAsVtx"), grpFlags);
    fFlags[kPropSkipProjection] = new QCheckBox(tr("Skip Projection"), grpFlags);
    fFlags[kPropNoShadow] = new QCheckBox(tr("No Shadow"), grpFlags);
    fFlags[kPropForceShadow] = new QCheckBox(tr("Force Shadow"), grpFlags);
    fFlags[kPropDisableNormal] = new QCheckBox(tr("Disable Normal"), grpFlags);
    fFlags[kPropCharacter] = new QCheckBox(tr("Character"), grpFlags);
    fFlags[kPartialSort] = new QCheckBox(tr("Partial Sort"), grpFlags);
    fFlags[kVisLOS] = new QCheckBox(tr("Vis LOS"), grpFlags);

    fLightingType = new QComboBox(grpFlags);
    fLightingType->addItems(QStringList() << "Material Lighting" << "Vertex Preshaded Lighting" << "Vertex Non-Preshaded Lighting" << "Projection" << "Shadow Erase" << "Shadow");
    fLightingType->setCurrentIndex(0);

    int lighting = (fData->fIcicle->getProps() & 0x3E00) >> 8;
    for (int i = 0; i < 6; i++) {
        if (lighting & (1 << i)) {
            fLightingType->setCurrentIndex(i);
        }
    }

    fFlags[kPropNoDraw]->setChecked((fData->fIcicle->getProps() & plSpan::kPropNoDraw) != 0);
    fFlags[kPropNoShadowCast]->setChecked((fData->fIcicle->getProps() & plSpan::kPropNoShadowCast) != 0);
    fFlags[kPropFacesSortable]->setChecked((fData->fIcicle->getProps() & plSpan::kPropFacesSortable) != 0);
    fFlags[kPropVolatile]->setChecked((fData->fIcicle->getProps() & plSpan::kPropVolatile) != 0);
    fFlags[kWaterHeight]->setChecked((fData->fIcicle->getProps() & plSpan::kWaterHeight) != 0);
    fFlags[kPropRunTimeLight]->setChecked((fData->fIcicle->getProps() & plSpan::kPropRunTimeLight) != 0);
    fFlags[kPropReverseSort]->setChecked((fData->fIcicle->getProps() & plSpan::kPropReverseSort) != 0);
    fFlags[kPropHasPermaLights]->setChecked((fData->fIcicle->getProps() & plSpan::kPropHasPermaLights) != 0);
    fFlags[kPropHasPermaProjs]->setChecked((fData->fIcicle->getProps() & plSpan::kPropHasPermaProjs) != 0);
    fFlags[kPropMatHasSpecular]->setChecked((fData->fIcicle->getProps() & plSpan::kPropMatHasSpecular) != 0);
    fFlags[kPropProjAsVtx]->setChecked((fData->fIcicle->getProps() & plSpan::kPropProjAsVtx) != 0);
    fFlags[kPropSkipProjection]->setChecked((fData->fIcicle->getProps() & plSpan::kPropSkipProjection) != 0);
    fFlags[kPropNoShadow]->setChecked((fData->fIcicle->getProps() & plSpan::kPropNoShadow) != 0);
    fFlags[kPropForceShadow]->setChecked((fData->fIcicle->getProps() & plSpan::kPropForceShadow) != 0);
    fFlags[kPropDisableNormal]->setChecked((fData->fIcicle->getProps() & plSpan::kPropDisableNormal) != 0);
    fFlags[kPropCharacter]->setChecked((fData->fIcicle->getProps() & plSpan::kPropCharacter) != 0);
    fFlags[kPartialSort]->setChecked((fData->fIcicle->getProps() & plSpan::kPartialSort) != 0);
    fFlags[kVisLOS]->setChecked((fData->fIcicle->getProps() & plSpan::kVisLOS) != 0);

    QGridLayout* layFlags = new QGridLayout(grpFlags);
    layFlags->setVerticalSpacing(0);
    layFlags->setHorizontalSpacing(8);
    layFlags->addWidget(fFlags[kPropNoDraw], 0, 0);
    layFlags->addWidget(fFlags[kPropNoShadowCast], 0, 1);
    layFlags->addWidget(fFlags[kPropFacesSortable], 0, 2);
    layFlags->addWidget(fFlags[kPropVolatile], 0, 3);
    layFlags->addWidget(fFlags[kWaterHeight], 1, 0);
    layFlags->addWidget(fFlags[kPropRunTimeLight], 1, 1);
    layFlags->addWidget(fFlags[kPropReverseSort], 1, 2);
    layFlags->addWidget(fFlags[kPropHasPermaLights], 1, 3);
    layFlags->addWidget(fFlags[kPropHasPermaProjs], 2, 0);
    layFlags->addWidget(fFlags[kPropMatHasSpecular], 2, 1);
    layFlags->addWidget(fFlags[kPropProjAsVtx], 2, 2);
    layFlags->addWidget(fFlags[kPropSkipProjection], 2, 3);
    layFlags->addWidget(fFlags[kPropNoShadow], 3, 0);
    layFlags->addWidget(fFlags[kPropForceShadow], 3, 1);
    layFlags->addWidget(fFlags[kPropDisableNormal], 3, 2);
    layFlags->addWidget(fFlags[kPropCharacter], 3, 3);
    layFlags->addWidget(fLightingType, 4, 0, 1, 2);
    layFlags->addWidget(fFlags[kPartialSort], 4, 2);
    layFlags->addWidget(fFlags[kVisLOS], 4, 3);

    QTabWidget* childTab = new QTabWidget(this);

    fLights = new QKeyList(fData->fDIOwner, childTab);
    for (size_t i=0; i<fData->fIcicle->getPermaLights().size(); i++)
        fLights->addKey(fData->fIcicle->getPermaLights()[i]);

    fProjs = new QKeyList(fData->fDIOwner, childTab);
    for (size_t i=0; i<fData->fIcicle->getPermaProjs().size(); i++)
        fProjs->addKey(fData->fIcicle->getPermaProjs()[i]);

    childTab->addTab(fLights, tr("Perma Lights"));
    childTab->addTab(fProjs, tr("Perma Projs"));


    fMatLink = new QCreatableLink(this);
    fMatLink->setText(tr("Material"));
    fMatLink->setKey(fData->fMaterial);

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(grpFlags, 0, 0, 1, 2);
    layout->addWidget(childTab, 1, 0);
    layout->addWidget(fMatLink, 2, 0, 1, 2);

    setWindowTitle("plIcicle");
}

void QIcicle::saveDamage()
{
    // Yeah... not happening >_>
}
