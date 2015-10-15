require "ai.constants"

function increasePopulation (parentnode)
	local parallel = parentnode:addNode("Parallel", "increasepopulation")
	parallel:setCondition("And(Not(IsOnCooldown{".. INCREASE_COOLDOWNID .."}),Filter(SelectIncreasePartner{".. INCREASE_COOLDOWNID .."}))")

		parallel:addNode("Steer(SelectionSeek)", "followincreasepartner")
		local spawn = parallel:addNode("Parallel", "spawn")
		spawn:setCondition("IsCloseToSelection{1}")

			spawn:addNode("Spawn", "spawn")
			spawn:addNode("TriggerCooldown{".. INCREASE_COOLDOWNID .."}", "increasecooldown")
			spawn:addNode("TriggerCooldownOnSelection{".. INCREASE_COOLDOWNID .."}", "increasecooldownonpartner")
end

function hunt (parentnode)
	local parallel = parentnode:addNode("Parallel", "hunt")
	parallel:setCondition("Not(IsOnCooldown{".. HUNT_COOLDOWNID .."})")

		parallel:addNode("Steer(SelectionSeek)", "follow"):setCondition("Filter(SelectPrey{ANIMAL_RABBIT})")
		parallel:addNode("AttackOnSelection", "attack"):setCondition("IsCloseToSelection{1}")
		parallel:addNode("SetPointOfInterest", "setpoi"):setCondition("IsCloseToSelection{1}")
		parallel:addNode("TriggerCooldown{".. HUNT_COOLDOWNID .."}", "increasecooldown"):setCondition("Not(IsSelectionAlive)")
end

function idle (parentnode)
	local prio = parentnode:addNode("PrioritySelector", "walkuncrowded")

		-- if there are too many objects (see parameter) visible of either the same npc type or the max count, leave the area
		-- otherwise walk randomly around in the area around your home position
		--prio:addNode("Steer(WanderAroundHome{100})", "wanderathome"):addCondition("Not(IsCrowded{10, 100})")
		-- if we can't walk in our home base area, we are wandering freely around to find another not crowded area
		prio:addNode("Steer(Wander)", "wanderfreely")
end

function die (parentnode)
end
