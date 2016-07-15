/**
 * @file
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <libpq-fe.h>
#include "core/Common.h"

namespace persistence {

class Connection;

class Model {
public:
	// don't change the order - code generator relies on this
	enum ConstraintType {
		UNIQUE = 1 << 0,
		PRIMARYKEY = 1 << 1,
		AUTOINCREMENT = 1 << 2,
		NOTNULL = 1 << 3
	};
	static constexpr int MAX_CONSTRAINTTYPES = 4;

	// don't change the order - code generator relies on this
	enum FieldType {
		STRING,
		LONG,
		INT,
		PASSWORD
	};
	static constexpr int MAX_FIELDTYPES = 4;

	struct Field {
		std::string name;
		FieldType type = Model::STRING;
		// bitmask from ConstraintType
		int contraintMask = 0;
		std::string defaultVal = "";
		int length = 0;
		intptr_t offset = 0;
	};
	typedef std::vector<Field> Fields;

	class State {
	public:
		State(PGresult* res);
		~State();

		PGresult* res = nullptr;
		std::string lastErrorMsg;
		ExecStatusType lastState = PGRES_FATAL_ERROR;
		int affectedRows = -1;
		bool result = false;
	};
protected:
	Fields _fields;

	Field getField(const std::string& name) const;

	const std::string _tableName;

	bool checkLastResult(State& state, Connection* connection) const;

public:
	Model(const std::string& tableName);

	const std::string& getTableName() const;

	const Fields& getFields() const;

	bool isPrimaryKey(const std::string& fieldname) const;

	class PreparedStatement {
	private:
		Model* _model;
		std::string _name;
		std::string _statement;
		std::vector<std::string> _params;
	public:
		PreparedStatement(Model* model, const std::string& name, const std::string& statement, const std::vector<std::string>& params = std::vector<std::string>());

		template<class Type>
		PreparedStatement& add(const Type& type) {
			_params.push_back(std::to_string(type));
			return *this;
		}

		PreparedStatement& add(const std::string& type) {
			_params.push_back(type);
			return *this;
		}

		State exec();
	};

	template<class TYPE>
	inline void setValue(const Field& f, const TYPE& value) {
		uint8_t* target = (uint8_t*)(this + f.offset);
		TYPE* targetValue = (TYPE*)target;
		*targetValue = value;
	}

	PreparedStatement prepare(const std::string& name, const std::string& statement);

	bool exec(const std::string& query);

	bool exec(const char* query);
};

inline bool Model::exec(const std::string& query) {
	return exec(query.c_str());
}

inline const std::string& Model::getTableName() const {
	return _tableName;
}

inline const Model::Fields& Model::getFields() const {
	return _fields;
}

}