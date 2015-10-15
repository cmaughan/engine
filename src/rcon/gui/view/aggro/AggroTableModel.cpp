#include "AggroTableModel.h"
#include <QColor>

namespace ai {
namespace debug {

AggroTableModel::AggroTableModel(const AIDebugger& debugger, QTableView *parent) :
		QAbstractTableModel(nullptr), _debugger(debugger), _parent(parent) {
}

AggroTableModel::~AggroTableModel() {
}

void AggroTableModel::update() {
	beginResetModel();
	endResetModel();
}

int AggroTableModel::rowCount(const QModelIndex & /*parent*/) const {
	const std::vector<AIStateAggroEntry>& aggro = _debugger.getAggro();
	return aggro.size();
}

int AggroTableModel::columnCount(const QModelIndex & /*parent*/) const {
	return 2;
}

QVariant AggroTableModel::headerData(int section, Qt::Orientation orientation,
		int role) const {
	if (orientation != Qt::Horizontal)
		return QVariant();

	if (role == Qt::DisplayRole) {
		switch (section) {
		case 0:
			return tr("ID");
		case 1:
			return tr("Aggro");
		default:
			break;
		}
	}
	return QVariant();
}

QVariant AggroTableModel::data(const QModelIndex &index, int role) const {
	const std::vector<AIStateAggroEntry>& aggro = _debugger.getAggro();
	if (role == Qt::DisplayRole) {
		switch (index.column()) {
		case 0:
			return aggro.at(index.row()).id;
		case 1:
			return aggro.at(index.row()).aggro;
		default:
			break;
		}
	}
	return QVariant();
}

}
}
