/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include "backend/entity/EntityStorage.h"

using namespace ai;

namespace backend {

/**
 * @ingroup AI
 */
class IsCloseToSelection: public ICondition {
protected:
	int _distance;

public:
	IsCloseToSelection(const std::string& parameters) :
			ICondition("IsCloseToSelection", parameters) {
		if (_parameters.empty()) {
			_distance = 1;
		} else {
			_distance = std::stoi(_parameters);
		}
	}
	CONDITION_FACTORY(IsCloseToSelection)

	bool evaluate(const AIPtr& entity) override {
		ai::Zone* zone = entity->getZone();
		if (zone == nullptr) {
			return false;
		}

		const FilteredEntities& selection = entity->getFilteredEntities();
		if (selection.empty()) {
			return false;
		}

		for (CharacterId id : selection) {
			const AIPtr& ai = zone->getAI(id);
			const Npc& npc = ai->getCharacterCast<AICharacter>().getNpc();
			const glm::vec3& pos = npc.pos();
			const glm::vec3& ownPos = entity->getCharacter()->getPosition();
			const float distance = glm::distance(pos, ownPos);
			if (distance > _distance) {
				return false;
			}
		}
		return true;
	}
};

}
