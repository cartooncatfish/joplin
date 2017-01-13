#include "foldermodel.h"
#include "uuid.h"
#include "dispatcher.h"

using namespace jop;

FolderModel::FolderModel() : AbstractListModel(), orderBy_("title") {
	connect(&dispatcher(), SIGNAL(folderCreated(QString)), this, SLOT(dispatcher_folderCreated(QString)));
	connect(&dispatcher(), SIGNAL(folderUpdated(QString)), this, SLOT(dispatcher_folderUpdated(QString)));
	connect(&dispatcher(), SIGNAL(folderDeleted(QString)), this, SLOT(dispatcher_folderDeleted(QString)));
	connect(&dispatcher(), SIGNAL(allFoldersDeleted()), this, SLOT(dispatcher_allFoldersDeleted()));
}

QVariant FolderModel::data(const QModelIndex & index, int role) const {
	Folder* folder = NULL;

	if (virtualItemShown() && index.row() == rowCount() - 1) {
		if (role == Qt::DisplayRole) return "Untitled";
		return "";
		//folder->setValue("title", BaseModel::Value(QString("Untitled")));
	} else {
		folder = atIndex(index.row());
	}

	if (role == Qt::DisplayRole) {
		return folder->value("title").toQVariant();
	}

	if (role == IdRole) {
		return folder->id().toQVariant();
	}

	return QVariant();
}

bool FolderModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	Folder* folder = atIndex(index.row());
	if (!folder) return false;

	if (role == Qt::EditRole) {
		folder->setValue("title", value);
		if (!folder->save()) return false;
		cacheClear();
		return true;
	}

	qWarning() << "Unsupported role" << role;
	return false;
}

Folder* FolderModel::atIndex(int index) const {
	if (cacheSize()) {
		if (index < 0 || index >= cacheSize()) {
			qWarning() << "Invalid folder index:" << index;
			return NULL;
		}

		return (Folder*)cacheGet(index);
	}

	cacheClear();

	cache_ = Folder::all(orderBy_);

	if (!cacheSize()) {
		qWarning() << "Invalid folder index:" << index;
		return NULL;
	} else {
		return atIndex(index);
	}
}

Folder* FolderModel::atIndex(const QModelIndex &index) const {
	return atIndex(index.row());
}

int FolderModel::baseModelCount() const {
	return Folder::count();
}

BaseModel *FolderModel::cacheGet(int index) const {
	return cache_[index].get();
}

void FolderModel::cacheSet(int index, BaseModel* baseModel) const {
	Folder* folder = static_cast<Folder*>(baseModel);
	cache_[index] = std::unique_ptr<Folder>(folder);
}

bool FolderModel::cacheIsset(int index) const {
	return index > 0 && cache_.size() > index;
}

void FolderModel::cacheClear() const {
	cache_.clear();
}

int FolderModel::cacheSize() const {
	return cache_.size();
}

QString FolderModel::indexToId(int index) const {
	return data(this->index(index), IdRole).toString();
}

int FolderModel::idToIndex(const QString &id) const {
	int count = this->rowCount();
	for (int i = 0; i < count; i++) {
		Folder* folder = atIndex(i);
		if (folder->idString() == id) return i;
	}
	return -1;
}

QString FolderModel::lastInsertId() const {
	return lastInsertId_;
}

bool FolderModel::setData(int index, const QVariant &value, int role) {
	return setData(this->index(index), value, role);
}

void FolderModel::addData(const QString &title) {
	Folder folder;
	folder.setValue("title", title);
	if (!folder.save()) return;

	lastInsertId_ = folder.id().toString();
}

void FolderModel::deleteData(const int index) {
	Folder* folder = atIndex(index);
	if (!folder) return;
	folder->dispose();
}

// TODO: instead of clearing the whole cache every time, the individual items
// could be created/updated/deleted

void FolderModel::dispatcher_folderCreated(const QString &folderId) {
	qDebug() << "FolderModel Folder created" << folderId;

	cacheClear();

	int from = 0;
	int to = rowCount() - 1;

	QVector<int> roles;
	roles << Qt::DisplayRole;

	// Necessary to make sure a new item is added to the view, even
	// though it might not be positioned there due to sorting
	beginInsertRows(QModelIndex(), to, to);
	endInsertRows();

	emit dataChanged(this->index(from), this->index(to), roles);
}

void FolderModel::dispatcher_folderUpdated(const QString &folderId) {
	qDebug() << "FolderModel Folder udpated" << folderId;

	cacheClear();

	QVector<int> roles;
	roles << Qt::DisplayRole;
	emit dataChanged(this->index(0), this->index(rowCount() - 1), roles);
}

void FolderModel::dispatcher_folderDeleted(const QString &folderId) {
	qDebug() << "FolderModel Folder deleted" << folderId;

	int index = idToIndex(folderId);
	if (index < 0) return;

	cacheClear();

	beginRemoveRows(QModelIndex(), index, index);
	endRemoveRows();
}

void FolderModel::dispatcher_allFoldersDeleted() {
	qDebug() << "FolderModel All folders deleted";
	cacheClear();
	beginResetModel();
	endResetModel();
}