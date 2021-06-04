﻿#pragma once


DEFINE_COMMAND_PLUGIN(ShowingOffDisable, "Does the same thing as vanilla Disable. For showing off!", 1, 1, kParams_OneOptionalInt);
DEFINE_COMMAND_PLUGIN(ShowingOffEnable, "Does the same thing as vanilla Enable. For showing off!", 1, 1, kParams_OneOptionalInt);
DEFINE_COMMAND_ALT_PLUGIN(ListAddList, AddFormListToFormList, "", 0, 3, kParams_TwoFormLists_OneOptionalIndex);
DEFINE_COMMAND_PLUGIN(MessageExAltShowoff, , 0, 22, kParams_JIP_OneFloat_OneInt_OneFormatString);
DEFINE_CMD_ALT_COND_PLUGIN(IsCornerMessageDisplayed, , "Returns 1/0 depending on if a corner message is displayed.", 0, NULL);
DEFINE_CMD_ALT_COND_PLUGIN(GetNumQueuedCornerMessages, , , 0, NULL);
DEFINE_CMD_ALT_COND_PLUGIN(IsAnimPlayingExCond, , "Same as IsAnimPlayingEx, but available as a condition. Had to cut the variationFlags filter.", 1, kParams_JIP_OneInt_OneOptionalInt);
DEFINE_COMMAND_PLUGIN(GetRadiationExtraData, , 1, 0, NULL);
DEFINE_COMMAND_PLUGIN(SetRadiationExtraData, , 1, 1, kParams_OneFloat);
DEFINE_COMMAND_PLUGIN(PlayerHasNightVision, , 0, 0, NULL);
DEFINE_COMMAND_PLUGIN(PlayerIsUsingTurbo, , 0, 0, NULL);
DEFINE_COMMAND_PLUGIN(PlayerHasCateyeEnabled, , 0, 0, NULL);
DEFINE_COMMAND_PLUGIN(PlayerHasImprovedSpotting, , 0, 0, NULL);
DEFINE_COMMAND_PLUGIN(PlayerIsDrinkingPlacedWater, , 0, 0, NULL);
DEFINE_COMMAND_PLUGIN(SetPlayerIsAMurderer, , 0, 1, kParams_OneInt);
DEFINE_CMD_ALT_COND_PLUGIN(IsNight, , "Returns true if it's night according to the current (or specified) climate.", 0, kParams_OneOptionalForm);
DEFINE_CMD_ALT_COND_PLUGIN(IsLimbCrippled, , "If no args are passed / arg is -1, returns true if actor has any crippled limbs. Otherwise, checks if the specified limb is crippled.", 1, kParams_TwoOptionalInts);
DEFINE_CMD_ALT_COND_PLUGIN(GetNumCrippledLimbs, , , 1, kParams_OneOptionalInt);
DEFINE_CMD_ALT_COND_PLUGIN(GetCrippledLimbsAsBitMask, , , 1, kParams_OneOptionalInt);
DEFINE_CMD_ALT_COND_PLUGIN(GetNumBrokenEquippedItems, , , 1, kParams_TwoOptionalInts);
DEFINE_CMD_ALT_COND_PLUGIN(GetEquippedItemsAsBitMask, , , 1, NULL);
DEFINE_COMMAND_PLUGIN(ClearShowoffSavedData, "", 0, 1, kParams_OneInt);
DEFINE_CMD_ALT_COND_PLUGIN(GetBaseEquippedWeight, , , 1, kParams_TwoOptionalInts);
DEFINE_CMD_ALT_COND_PLUGIN(GetCalculatedEquippedWeight, , "Accounts for perk effects + weapon mods.", 1, kParams_TwoOptionalInts);
DEFINE_CMD_ALT_COND_PLUGIN(GetCalculatedMaxCarryWeight, GetMaxCarryWeightPerkModified, "Accounts for GetMaxCarryWeight perk entry.", 1, NULL);


bool(__cdecl* Cmd_Disable)(COMMAND_ARGS) = (bool(__cdecl*)(COMMAND_ARGS)) 0x5C45E0;
bool(__cdecl* Cmd_Enable)(COMMAND_ARGS) = (bool(__cdecl*)(COMMAND_ARGS)) 0x5C43D0;

bool Cmd_ShowingOffDisable_Execute(COMMAND_ARGS) {
	return Cmd_Disable(PASS_COMMAND_ARGS);
}

bool Cmd_ShowingOffEnable_Execute(COMMAND_ARGS) {
	return Cmd_Enable(PASS_COMMAND_ARGS);
}




//ripped code from FOSE's ListAddForm
bool Cmd_ListAddList_Execute(COMMAND_ARGS)
{
	*result = 1;
	BGSListForm* pListForm = nullptr;
	BGSListForm* pToAppendList = nullptr;
	UInt32 addAtIndex = eListEnd;

	ExtractArgsEx(EXTRACT_ARGS_EX, &pListForm, &pToAppendList, &addAtIndex);
	if (!pListForm || !pToAppendList) return true;

	auto Try_Adding_Form_From_FormList = [&](TESForm* form)
	{
		UInt32 const addedAtIndex = pListForm->AddAt(form, addAtIndex);
		if (addedAtIndex == eListInvalid)
		{
			*result = 0; //error
			return false;
		}
		return true;
	};

	// Need to append elements to our own array in order to iterate backwards, to append in descending key order.
	std::vector<TESForm*> formArr;
	
	for (tList<TESForm>::Iterator iter = pToAppendList->list.Begin(); !iter.End(); ++iter)
	{
		TESForm* form = iter.Get();
		if (form)
		{
			if (addAtIndex != eListEnd)
			{
				formArr.push_back(form);
			}
			else
			{
				if (!Try_Adding_Form_From_FormList(form)) break;
			}
		}
		else
		{
			_ERROR("ListAddList - found invalid form in formlist.");
			*result = 0;
			break;
		}
	}

	if (!formArr.empty())
	{
		for (int i = formArr.size() - 1; i >= 0; i--)
		{
			if (!Try_Adding_Form_From_FormList(formArr[i])) break;
		}
	}

	return true;
}

const UInt32 kMsgIconsPathAddr[] = { 0x10208A0, 0x10208E0, 0x1025CDC, 0x1030E78, 0x103A830, 0x1049638, 0x104BFE8 };

//99% ripped from JIP's MessageExAlt.
bool Cmd_MessageExAltShowoff_Execute(COMMAND_ARGS)
{
	float displayTime;
	UINT32 appendToQueue;
	char* buffer = GetStrArgBuffer();
	if (!ExtractFormatStringArgs(2, buffer, EXTRACT_ARGS_EX, kCommandInfo_MessageExAltShowoff.numParams, &displayTime, &appendToQueue))
		return true;

	char* msgPtr = GetNextTokenJIP(buffer, '|');
	msgPtr[0x203] = 0;
	if (*msgPtr)
	{
		if ((buffer[0] == '#') && (buffer[1] >= '0') && (buffer[1] <= '6') && !buffer[2])
			QueueUIMessage(msgPtr, 0, (const char*)kMsgIconsPathAddr[buffer[1] - '0'], NULL, displayTime, appendToQueue != 0);
		else QueueUIMessage(msgPtr, 0, buffer, NULL, displayTime, appendToQueue != 0);
	}
	else QueueUIMessage(buffer, 0, NULL, NULL, displayTime, appendToQueue != 0);

	return true;
}


bool Cmd_IsCornerMessageDisplayed_Execute(COMMAND_ARGS)
{
	*result = !g_HUDMainMenu->queuedMessages.Empty();  
	//*result = (bool)g_HUDMainMenu->currMsgKey;
	//another way to check. Seems to be a bit slower to update when initially adding a message to the queue.
	return true;
}
bool Cmd_IsCornerMessageDisplayed_Eval(COMMAND_ARGS_EVAL)
{
	*result = !g_HUDMainMenu->queuedMessages.Empty();
	return true;
}

bool Cmd_GetNumQueuedCornerMessages_Execute(COMMAND_ARGS)
{
	*result = g_HUDMainMenu->queuedMessages.Count();
	return true;
}
bool Cmd_GetNumQueuedCornerMessages_Eval(COMMAND_ARGS_EVAL)
{
	*result = g_HUDMainMenu->queuedMessages.Count();
	return true;
}


//Code ripped from JIP's IsAnimPlayingEx
bool Cmd_IsAnimPlayingExCond_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	auto category = (UInt32)arg1;
	auto subType = (UInt32)arg2;  //optional

	if (!thisObj) return true;
	if (category > 5 || category < 1) return true;
	if (subType > 23) return true;

	AnimData* animData = thisObj->GetAnimData();
	if (!animData) return true;
	const AnimGroupClassify* classify;
	for (UInt16 groupID : animData->animGroupIDs)
	{
		UInt32 const animID = groupID & 0xFF;
		if (animID >= 245) continue;
		classify = &s_animGroupClassify[animID];
		if (classify->category == category && (category >= 4 || (!subType || classify->subType == subType)))
		{
			*result = 1;
			break;
		}
	}

	return true;
}
bool Cmd_IsAnimPlayingExCond_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 category, subType = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &category, &subType)) return true;

	return Cmd_IsAnimPlayingExCond_Eval(thisObj, (void*)category, (void*)subType, result);
}

bool Cmd_GetRadiationExtraData_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (thisObj)
	{
		ExtraRadiation* xRadius = GetExtraTypeJIP(&thisObj->extraDataList, Radiation);
		if (xRadius) *result = xRadius->radiation;
	}
	return true;
}


bool Cmd_SetRadiationExtraData_Execute(COMMAND_ARGS)
{
	float fNewVal = 0;
	*result = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &fNewVal)) return true;
	if (thisObj)
	{
		ThisStdCall(0x422350, &thisObj->extraDataList, fNewVal);
		*result = 1;
	}
	return true;
}


bool Cmd_PlayerHasNightVision_Execute(COMMAND_ARGS)
{
	*result = g_thePlayer->hasNightVisionApplied;
	return true;
}


bool Cmd_PlayerIsUsingTurbo_Execute(COMMAND_ARGS)
{
	*result = g_thePlayer->isUsingTurbo;  
	return true;
}


bool Cmd_PlayerHasCateyeEnabled_Execute(COMMAND_ARGS)
{
	*result = g_thePlayer->isCateyeEnabled;
	return true;
}


bool Cmd_PlayerHasImprovedSpotting_Execute(COMMAND_ARGS)
{
	*result = g_thePlayer->isSpottingImprovedActive;
	return true;
}


bool Cmd_PlayerIsDrinkingPlacedWater_Execute(COMMAND_ARGS)
{
	*result = g_thePlayer->isDrinkingPlacedWater;
	return true;
}


bool Cmd_SetPlayerIsAMurderer_Execute(COMMAND_ARGS)
{
	UInt32 bIsMurderer = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &bIsMurderer)) return true;
	g_thePlayer->bIsAMurderer = (bIsMurderer != 0);
	return true;
}


bool Cmd_IsNight_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESClimate* climate = (TESClimate*)arg1;
	Sky* sky = *g_currentSky;
	float const gameHour = ThisStdCall<double>(0x966A20, sky);
	float sunrise, sunset;
	if (climate && IS_TYPE(climate, TESClimate))
	{
		sunrise = ThisStdCall<UInt8>(0x595F10, climate, 1) / 6.0F;  //sunrise begin sprinkled with adjustments. 
		sunset = ThisStdCall<UInt8>(0x595F10, climate, 2) / 6.0F;  //Second arg determines which type of time to check.
	}
	else
	{
		sunrise = ThisStdCall<double>(0x595F50, sky);
		sunset = ThisStdCall<double>(0x595FC0, sky);
	}
	if (sunset <= gameHour || (sunrise >= gameHour))
		*result = 1;
	return true;
}
bool Cmd_IsNight_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESClimate* climate = NULL;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &climate)) return true;
	return Cmd_IsNight_Eval(0, climate, 0, result);
}


bool Cmd_IsLimbCrippled_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	Actor* const actor = (Actor*)thisObj;
	UInt32 limbID = (UInt32)arg1;
	UInt32 const threshold = (UInt32)arg2;
	if (limbID == -1)
	{
		//loop through all limb health AVs and break if a single one is at 0 health (or below threshold).
		for (limbID = kAVCode_PerceptionCondition; limbID <= kAVCode_BrainCondition; limbID++)
		{
			if (actor->avOwner.GetActorValue(limbID) <= threshold)
			{
				*result = 1;
				break;
			}
		}
	}
	else if (limbID <= 6)
	{
		limbID += kAVCode_PerceptionCondition;
		if (actor->avOwner.GetActorValue(limbID) <= threshold) *result = 1;
	}
	return true;
}
bool Cmd_IsLimbCrippled_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 limbID = -1;
	UInt32 threshold = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &limbID, &threshold)) return true;
	return Cmd_IsLimbCrippled_Eval(thisObj, (void*)limbID, (void*)threshold, result);
}

bool Cmd_GetNumCrippledLimbs_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	Actor* const actor = (Actor*)thisObj;
	UInt32 const threshold = (UInt32)arg1;
	UInt32 numCrippledLimbs = 0;
	for (UInt32 limbID = kAVCode_PerceptionCondition; limbID <= kAVCode_BrainCondition; limbID++)
	{
		if (actor->avOwner.GetActorValue(limbID) <= threshold)
		{
			numCrippledLimbs++;
		}
	}
	*result = numCrippledLimbs;
	return true;
}
bool Cmd_GetNumCrippledLimbs_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 threshold = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &threshold)) return true;
	return Cmd_GetNumCrippledLimbs_Eval(thisObj, (void*)threshold, 0, result);
}


bool Cmd_GetCrippledLimbsAsBitMask_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	Actor* const actor = (Actor*)thisObj;
	UInt32 const threshold = (UInt32)arg1;

	enum LimbGoneFlags
	{
		kFlag_Head = 1,
		kFlag_Torso = 2,
		kFlag_LeftArm = 4,
		kFlag_RightArm = 8,
		kFlag_LeftLeg = 0x10,
		kFlag_RightLeg = 0x20,
		kFlag_Brain = 0x40,
	};
	UInt32 flagsArr[7] = { kFlag_Head, kFlag_Torso, kFlag_LeftArm, kFlag_RightArm, kFlag_LeftLeg, kFlag_RightLeg, kFlag_Brain };

	UInt32 CrippledLimbsMask = 0;
	for (UInt32 limbID = kAVCode_PerceptionCondition; limbID <= kAVCode_BrainCondition; limbID++)
	{
		if (actor->avOwner.GetActorValue(limbID) <= threshold)
		{
			CrippledLimbsMask |= flagsArr[limbID - kAVCode_PerceptionCondition];
		}
	}
	*result = CrippledLimbsMask;
	return true;
}
bool Cmd_GetCrippledLimbsAsBitMask_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 threshold = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &threshold)) return true;
	return Cmd_GetCrippledLimbsAsBitMask_Eval(thisObj, (void*)threshold, 0, result);
}


// Get Equipped Object utility functions, from FOSE
class MatchBySlot : public FormMatcher
{
	UInt32 m_slotMask;
public:
	MatchBySlot(UInt32 slot) : m_slotMask(TESBipedModelForm::MaskForSlot(slot)) {}
	bool Matches(TESForm* pForm) const {
		UInt32 formMask = 0;
		if (pForm) {
			if (IS_TYPE(pForm, TESObjectWEAP)) {
				formMask = TESBipedModelForm::eSlot_Weapon;
			}
			else {
				TESBipedModelForm* pBip = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
				if (pBip) {
					formMask = pBip->partMask;
				}
			}
		}
		return (formMask & m_slotMask) != 0;
	}
};

class MatchBySlotMask : public FormMatcher
{
	UInt32 m_targetMask;
	UInt32 m_targetData;
public:
	MatchBySlotMask(UInt32 targetMask, UInt32 targetData) : m_targetMask(targetMask) {}
	bool Matches(TESForm* pForm) const {
		UInt32 slotMask = 0;
		if (pForm) {
			if (IS_TYPE(pForm, TESObjectWEAP)) {
				slotMask = TESBipedModelForm::ePart_Weapon;
			}
			else {
				TESBipedModelForm* pBip = DYNAMIC_CAST(pForm, TESForm, TESBipedModelForm);
				if (pBip) {
					slotMask = pBip->partMask;
				}
			}
		}
		return ((slotMask & m_targetMask) == m_targetData);
	}
};

static EquipData FindEquipped(TESObjectREFR* thisObj, FormMatcher& matcher) {
	ExtraContainerChanges* pContainerChanges = static_cast<ExtraContainerChanges*>(thisObj->extraDataList.GetByType(kExtraData_ContainerChanges));
	return (pContainerChanges) ? pContainerChanges->FindEquipped(matcher) : EquipData();
}

typedef TESBipedModelForm::EPartBit EquippedItemIndex;
typedef TESBipedModelForm::ESlot EquippedItemSlot;


bool Cmd_GetNumBrokenEquippedItems_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	UInt32 const threshold = (UInt32)arg1;
	UInt32 const flags = (UInt32)arg2;

	UInt32 numBrokenItems = 0;
	for (UInt32 slotIdx = EquippedItemIndex::ePart_Head; slotIdx <= EquippedItemIndex::ePart_BodyAddon3; slotIdx++)
	{
		MatchBySlot matcher(slotIdx);
		if (flags)  //if flags = 0 (default), every equip slot is checked.
		{
			//Otherwise, return if flag is not toggled on for item.
			if (flags && !(TESBipedModelForm::MaskForSlot(slotIdx) & flags)) continue;
		}
		EquipData equipD = FindEquipped(thisObj, matcher);
		if (equipD.pForm)
		{
			ExtraHealth* pXHealth = equipD.pExtraData ? (ExtraHealth*)equipD.pExtraData->GetByType(kExtraData_Health) : NULL;
			if (pXHealth)
			{
				if (pXHealth->health <= threshold) numBrokenItems++;
			}
			else
			{
				TESHealthForm* pHealth = DYNAMIC_CAST(equipD.pForm, TESForm, TESHealthForm);
				if (pHealth)
					if (pHealth->health <= threshold) numBrokenItems++;
			}
		}
	}
	*result = numBrokenItems;
	return true;
}
bool Cmd_GetNumBrokenEquippedItems_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 threshold = 0;
	UInt32 flags = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &threshold, &flags)) return true;
	return Cmd_GetNumBrokenEquippedItems_Eval(thisObj, (void*)threshold, (void*)flags, result);
}


bool Cmd_GetEquippedItemsAsBitMask_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	UInt32 flags = 0;
	for (UInt32 slotIdx = EquippedItemIndex::ePart_Head; slotIdx <= EquippedItemIndex::ePart_BodyAddon3; slotIdx++)
	{
		MatchBySlot matcher(slotIdx);
		EquipData equipD = FindEquipped(thisObj, matcher);
		if (equipD.pForm)
		{
			flags |= TESBipedModelForm::MaskForSlot(slotIdx);
		}
	}
	*result = flags;
	return true;
}
bool Cmd_GetEquippedItemsAsBitMask_Execute(COMMAND_ARGS)
{
	return Cmd_GetEquippedItemsAsBitMask_Eval(thisObj, 0, 0, result);
}

#if 0
DEFINE_COMMAND_PLUGIN(UnequipItemsFromBitMask, , 1, 1, kParams_OneInt);
bool Cmd_UnequipItemsFromBitMask_Execute(COMMAND_ARGS)
{
	UInt32 flags = 0, noEquip = 0, noMessage = 1;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &flags, &noEquip, &noMessage)) return true;
	if (!IS_ACTOR(thisObj)) return true;
	Actor* actor = (Actor*)thisObj;
	for (UInt32 slotIdx = EquippedItemIndex::ePart_Head; slotIdx <= EquippedItemIndex::ePart_BodyAddon3; slotIdx++)
	{
		MatchBySlot matcher(slotIdx);
		EquipData equipD = FindEquipped(thisObj, matcher);
		if (equipD.pForm)
		{
			if (flags & TESBipedModelForm::MaskForSlot(slotIdx))
			{
				//Unequip item if its matching flag is set.
				actor->UnequipItem(equipD.pForm, 1, xData, 1, noEquip != 0, noMessage != 0);
			}
		}
	}
	return true;
}
#endif


bool Cmd_ClearShowoffSavedData_Execute(COMMAND_ARGS)
{
	UInt32 auxStringMaps;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &auxStringMaps)) return true;
	UInt8 modIdx = scriptObj->GetOverridingModIdx();
	if (auxStringMaps && s_auxStringMapArraysPerm.Erase((auxStringMaps == 2) ? 0xFF : modIdx))
		s_dataChangedFlags |= kChangedFlag_AuxStringMaps;
	return true;
}


bool Cmd_GetBaseEquippedWeight_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	UInt32 const minWeight = (UInt32)arg1;
	UInt32 const flags = (UInt32)arg2;

	float totalWeight = 0;
	for (UInt32 slotIdx = EquippedItemIndex::ePart_Head; slotIdx <= EquippedItemIndex::ePart_BodyAddon3; slotIdx++)
	{
		MatchBySlot matcher(slotIdx);
		if (flags)  //if flags = 0 (default), every equip slot is checked.
		{
			//Otherwise, return if flag is not toggled on for item.
			if (flags && !(TESBipedModelForm::MaskForSlot(slotIdx) & flags)) continue;
		}
		EquipData equipD = FindEquipped(thisObj, matcher);
		if (equipD.pForm)
		{
			TESWeightForm* pWeight = DYNAMIC_CAST(equipD.pForm, TESForm, TESWeightForm);
			if (pWeight)
			{
				if (pWeight->weight >= minWeight)
				{
					totalWeight += pWeight->weight;
				}
			}
		}
	}
	*result = totalWeight;
	return true;
}
bool Cmd_GetBaseEquippedWeight_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 minWeight = 0;
	UInt32 flags = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &minWeight, &flags)) return true;
	return Cmd_GetBaseEquippedWeight_Eval(thisObj, (void*)minWeight, (void*)flags, result);
}


bool Cmd_GetCalculatedEquippedWeight_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	UInt32 const minWeight = (UInt32)arg1;
	UInt32 const flags = (UInt32)arg2;
	Actor* actor = (Actor*)thisObj;

	float totalWeight = 0;
	bool isHardcore = g_thePlayer->isHardcore;
	
	for (UInt32 slotIdx = EquippedItemIndex::ePart_Head; slotIdx <= EquippedItemIndex::ePart_BodyAddon3; slotIdx++)
	{
		if (flags)  //if flags = 0 (default), every equip slot is checked.
		{
			//Otherwise, return if flag is not toggled on for item.
			if (flags && !(TESBipedModelForm::MaskForSlot(slotIdx) & flags)) continue;
		}
		
		float itemWeight = 0;
		TESForm* item = nullptr;
		bool isWeapon = false;
		ContChangesEntry* weapInfo = nullptr;

		if (slotIdx == EquippedItemIndex::ePart_Weapon) {
			isWeapon = true;
			weapInfo = actor->baseProcess->GetWeaponInfo();
			item = weapInfo->type;
			if (item) {
				bool hasDecreaseWeightMod = ThisStdCall<bool>(0x4BDA70, weapInfo, TESObjectWEAP::kWeaponModEffect_DecreaseWeight);
				itemWeight = ThisStdCall<double>(0x4BE380, (TESObjectWEAP*)item, hasDecreaseWeightMod);
				if (itemWeight >= 10.0)
				{
					float heavyWeaponWeightMult = 1.0;
					ApplyPerkModifiers(kPerkEntry_AdjustHeavyWeaponWeight, (Actor*)g_thePlayer, &heavyWeaponWeightMult);
					itemWeight = itemWeight * heavyWeaponWeightMult;
				}
			}
		}
		else {
			MatchBySlot matcher(slotIdx);
			EquipData equipD = FindEquipped(thisObj, matcher);
			item = equipD.pForm;
			if (item) {
				itemWeight = CdeclCall<double>(0x48EBC0, item, isHardcore);  // GetItemWeight. isHardcore check only affects ammo, but whatever.
			}
		}

		if (itemWeight > 0.0)
		{
			float hasPackRatFlt = 0.0;
			ApplyPerkModifiers(kPerkEntry_ModifyLightItems, (Actor*)g_thePlayer, &hasPackRatFlt);
			if (hasPackRatFlt > 0.0)
			{
				float const fPackRatThreshold = *(float*)(0x11C6478 + 4);
				float const fPackRatModifier = *(float*)(0x11C64A8 + 4);
				if (fPackRatThreshold >= (double)itemWeight)
					itemWeight = itemWeight * fPackRatModifier;
			}

#if 0		// todo: figure out wtf 0x4D0D83 does.
			if (isWeapon && weapInfo)
			{
				//NOTE: Game does some weird jank to account for multiple DecreaseWeight effects.
				//I just really have no idea what's going on at 0x4D0D83 .
				bool const hasDecreaseWeightEffect = ThisStdCall<bool>(0x4BDA70, weapInfo, TESObjectWEAP::kWeaponModEffect_DecreaseWeight);
				if (hasDecreaseWeightEffect) {
					itemWeight *= ThisStdCall<float>(0x4BCF60, weapInfo->type, TESObjectWEAP::kWeaponModEffect_DecreaseWeight, 1);
				}
			}
#endif
			
			if (itemWeight >= minWeight) totalWeight += itemWeight;
		}
	}
	*result = totalWeight;
	return true;
}
bool Cmd_GetCalculatedEquippedWeight_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 minWeight = 0;
	UInt32 flags = 0;
	if (!ExtractArgsEx(EXTRACT_ARGS_EX, &minWeight, &flags)) return true;
	return Cmd_GetCalculatedEquippedWeight_Eval(thisObj, (void*)minWeight, (void*)flags, result);
}



bool Cmd_GetCalculatedMaxCarryWeight_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!IS_ACTOR(thisObj)) return true;
	*result = ThisStdCall<double>(0x8A0C20, (Actor*)thisObj);
	return true;
}
bool Cmd_GetCalculatedMaxCarryWeight_Execute(COMMAND_ARGS)
{
	return Cmd_GetCalculatedMaxCarryWeight_Eval(thisObj, 0, 0, result);
}


#if _DEBUG




DEFINE_COMMAND_PLUGIN(SetCellFullNameAlt, "Like SetCellFullName but accepts a string.", 0, 2, kParams_JIP_OneCell_OneString);
bool Cmd_SetCellFullNameAlt_Execute(COMMAND_ARGS)
{
	TESObjectCELL* cell;
	char newName[256]; 
	
	ExtractArgsEx(EXTRACT_ARGS_EX, &cell, &newName);
	if (!cell) return true;

	TESFullName* fullName = &cell->fullName;  //wtf. Seems to work, idk why
	ThisStdCall(0x489100, fullName, newName);  //rip, needs something more to save-bake
	//fullName->name.Set(newName); //crashes
	
	return true;
}


DEFINE_COMMAND_PLUGIN(GetQueuedCornerMessages, "Returns the queued corner messages as a multidimensional array.", 0, 0, NULL);
bool Cmd_GetQueuedCornerMessages_Execute(COMMAND_ARGS)
{
	NVSEArrayVar* msgArr = g_arrInterface->CreateArray(NULL, 0, scriptObj);

	for (UINT32 iIndex = g_HUDMainMenu->queuedMessages.Count() + 1; ; --iIndex)
	{
		if (iIndex == 0) break;
		g_HUDMainMenu->queuedMessages.GetNthItem(iIndex);
		//no idea what to do with this :snig:
	}

	g_arrInterface->AssignCommandResult(msgArr, result);
	return true;
}


DEFINE_COMMAND_ALT_PLUGIN(SetBaseActorValue, SetBaseAV, , 0, 3, kParams_JIP_OneActorValue_OneFloat_OneOptionalActorBase); 
bool Cmd_SetBaseActorValue_Execute(COMMAND_ARGS) 
{
	UInt32 actorVal;
	float valueToSet;
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(EXTRACT_ARGS, &actorVal, &valueToSet, &actorBase)) return true;
	if (!actorBase)
	{
		if (!thisObj || !thisObj->IsActor()) return true;
		actorBase = (TESActorBase*)thisObj->baseForm;
	}
	UInt32 currentValue = *result = actorBase->avOwner.GetActorValue(actorVal);
	actorBase->ModActorValue(actorVal, (valueToSet - currentValue));
	return true;
}

DEFINE_COMMAND_PLUGIN(GetItemRefValue, , 1, 0, NULL);
bool Cmd_GetItemRefValue_Execute(COMMAND_ARGS)
{
	*result = -1;
	if (thisObj)
	{
		ContChangesEntry tempEntry(NULL, 1, thisObj->baseForm);  //copying after GetCalculatedWeaponDamage from JIP, and c6.
		ExtraContainerChanges::ExtendDataList extendData;
		extendData.Init(&thisObj->extraDataList);
		tempEntry.extendData = &extendData;
		*result = ThisStdCall<double>(0x4BD400, &tempEntry);
	}
	return true;
}

DEFINE_COMMAND_PLUGIN(GetItemRefHealth, , 1, 1, kParams_OneOptionalInt);
bool Cmd_GetItemRefHealth_Execute(COMMAND_ARGS)
{
	*result = -1;
	UINT32 bPercent = 0;
	if (thisObj && ExtractArgs(EXTRACT_ARGS, &bPercent))
	{
		ContChangesEntry tempEntry(NULL, 1, thisObj->baseForm);  //copying after GetCalculatedWeaponDamage from JIP, and c6.
		ExtraContainerChanges::ExtendDataList extendData;
		extendData.Init(&thisObj->extraDataList);
		tempEntry.extendData = &extendData;

		//double __thiscall ContChangesEntry::GetHealthPerc(ContChangesEntry * this, int a2)
		*result = ThisStdCall<double>(0x4BCDB0, &tempEntry, bPercent);
	}
	return true;
}

/*
DEFINE_COMMAND_PLUGIN(GetCalculatedItemWeight, , 1, 0, One);
bool Cmd_GetCalculatedItemWeight_Execute(COMMAND_ARGS)
{
	*result = -1;
	if (thisObj)
	{
		ContChangesEntry tempEntry(NULL, 1, thisObj->baseForm);  
		ExtraContainerChanges::ExtendDataList extendData;
		extendData.Init(&thisObj->extraDataList);
		tempEntry.extendData = &extendData;

		//double __thiscall ContChangesEntry::GetHealthPerc(ContChangesEntry * this, int a2)
		*result = ThisStdCall<double>(0x4BCDB0, &tempEntry, bPercent);
	}
	return true;
}
*/

struct ActivateRefEntry  //LinkedRefEntry, no way this will work..
{
	UInt32		linkID;
	UInt8		modIdx;

	void Set(UInt32 _linkID, UInt8 _modIdx)
	{
		linkID = _linkID;
		modIdx = _modIdx;
	}
};
UnorderedMap<UInt32, ActivateRefEntry> s_activateRefModified;
UnorderedMap<UInt32, UInt32> s_activateRefDefault, s_activateRefsTemp;


DEFINE_COMMAND_PLUGIN(SetEnableParent, , 1, 1, kParams_OneOptionalForm);

//Stole some code from JIP (TESObjectREFR::SetLinkedRef)
bool Cmd_SetEnableParent_Execute(COMMAND_ARGS)
{
	//TESObjectREFR* newParent = NULL;
	//if (!ExtractArgsEx(EXTRACT_ARGS_EX, &newParent)) return true;

#if 0
	ExtraDataList xData = thisObj->extraDataList;
	ExtraActivateRef* xActivateRef = GetExtraType(xData, ActivateRef);

	RemoveExtraData(&xData, xActivateRef, true);
	if (!newParent)
	{
		//xActivateRef->parentRefs.RemoveAll();  //DANGER! MUST TEST
		auto findDefID = s_activateRefDefault.Find(thisObj->refID);
		if (findDefID)
		{
			if (xActivateRef)
			{
				if (*findDefID)
				{
					TESForm* form = LookupFormByRefID(*findDefID);
					if (form && form->GetIsReference()) xLinkedRef->linkedRef = (TESObjectREFR*)form;
				}
				else RemoveExtraData(xData, xLinkedRef, true);
			}
			findDefID.Remove();
		}
		s_activateRefModified.Erase(thisObj->refID);
		return true;
	}
	else
	{

	}

	if (!linkObj)
	{
		auto findDefID = s_activateRefDefault.Find(refID);
		if (findDefID)
		{
			if (xLinkedRef)
			{
				if (*findDefID)
				{
					TESForm* form = LookupFormByRefID(*findDefID);
					if (form && form->GetIsReference()) xLinkedRef->linkedRef = (TESObjectREFR*)form;
				}
				else RemoveExtraData(&extraDataList, xLinkedRef, true);
			}
			findDefID.Remove();
		}
		s_activateRefModified.Erase(refID);
		return true;
	}
#endif
	return true;
}

double g_TestDemoVar = 0;

void __fastcall TestDemoFunc(double *stuff)
{
	*stuff -= 1;
}

DEFINE_COMMAND_PLUGIN(TestDemo, , 0, 0, NULL);
bool Cmd_TestDemo_Execute(COMMAND_ARGS)
{
	UInt32 bInt = 0;
	//if (!ExtractArgsEx(EXTRACT_ARGS_EX)) return true;
	*result = g_thePlayer->hasNightVisionApplied;
	return true;
}

#endif