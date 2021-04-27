#pragma once
#include "GameData.h"
#include "SafeWrite.h"
#include "internal/StewieMagic.h"
#include "GameProcess.h"

//idk
HMODULE ShowOffHandle;




//Singletons
PlayerCharacter* g_thePlayer = nullptr;
ActorValueOwner* g_playerAVOwner = NULL;
ProcessManager* g_processManager = nullptr;
InterfaceManager* g_interfaceManager = nullptr;
BSWin32Audio* g_bsWin32Audio = nullptr;
DataHandler* g_dataHandler = nullptr;
BSAudioManager* g_audioManager = nullptr;
Sky** g_currentSky = nullptr; 


//Globals - for INI globals, see settings.h
bool g_canPlayerPickpocketEqItems() { return *(UInt32*)0x75E87B != 0xFFD5F551; }
//Checks if the address has been changed, to take into account Stewie's Tweaks' implementation.
//If the address was changed by something else, uh... Well I don't take that into account.

bool g_canPlayerPickpocketInCombat = false;




//---HOOKS


void __fastcall ContainerMenuHandleMouseoverAlt(ContainerMenu* menu, void* edx, int a2, int a3)
{
	// if not in pickpocket mode return
	if (menu->mode != ContainerMenu::Mode::kPickpocket || !IS_TYPE(menu->containerRef, Character)) return;

	TileMenu* tileMenu = menu->tile;
	Tile* subtitlesTile = CdeclCall<Tile*>(0xA08B20, tileMenu, "CM_SUBTITLE");
	if (subtitlesTile) subtitlesTile->SetString(kTileValue_string, "", 1);
	ThisStdCall(0x75CF70, menu, a2, a3);
}

int CalculateCombatPickpocketAPCost(ContChangesEntry* item, Actor* target, signed int itemValue, signed int count, bool isItemOwnedByTarget)
{
	// Average player (5 AGL) will have ~80 action points
	// 7 AGL = ~86 action points. So the base is still around 80.
	// Meaning, to balance this out, AP costs should refrain from going over 80 (save for some exceptions).

	// Detection value won't be taken into account - this is a brute-force kind of approach, anyone will see it coming, unless they're unconscious.
	// Still, it's a Sleight of Hand trick, so the target's Perception will be important, as will the player's Sneak + Agility (used as an intermediary for Sleight of Hand).
	// If the enemy is agile, it'll be trickier for the player to grab things from them, so also take targetAgility into account.
	

	int playerAgility = g_playerAVOwner->GetThresholdedActorValue(kAVCode_Agility);
	int playerSneak = g_playerAVOwner->GetThresholdedActorValue(kAVCode_Sneak);
	int targetPerception = target->avOwner.GetThresholdedActorValue(kAVCode_Perception);
	int targetAgility = target->avOwner.GetThresholdedActorValue(kAVCode_Agility);

	float itemWeight = item->type->GetWeight() * count;


	float cost = //g_fPickpocketBaseChance()
		+ playerAgility //* g_fPickpocketPlayerAgilityMult
		+ playerSneak //* g_fPickpocketPlayerSneakMult
		- itemValue //* g_fPickpocketItemValueMult
		- itemWeight //* g_fPickpocketItemWeightMult
		;

	
	// If the item is equipped ON THE TARGET, the target's Strength will be taken into account to apply a penalty.
	// This penalty can be minimized with the player's own Strength, which will also only be taken into account if the item being stolen is equipped ON THE TARGET.
	// NOTE: Equipped items cannot be stolen in vanilla, for this to do anything, that feature must be enabled in an NVSE plugin.
	if (item->GetEquippedExtra() && isItemOwnedByTarget)
	{
		int targetStrength = target->avOwner.GetThresholdedActorValue(kAVCode_Strength);
		int playerStrength = g_playerAVOwner->GetThresholdedActorValue(kAVCode_Strength);

		//cost *= g_fPickpocketWornItemChanceMult;
	}

	//cost = min(cost, fCombatPickpocketMaxAPCost); //Cap it to 100 AP (by default).
	//cost = max(cost, fCombatPickPocketMinAPCost); //Costs at least 15 AP (by default).

	return cost;
}

// custom pickpocket code, for ripping items straight out of an opponent's hands/pockets.
bool __fastcall TryCombatPickpocket(ContChangesEntry* selection, SInt32 count, Actor* actor, signed int itemValue)
{
	bool wasSuccessful = true;
	ContainerMenu* menu = ContainerMenu::GetSingleton();

	if (actor && /*itemValue > 0 &&*/ !actor->GetHasKnockedState())
	{
		bool isItemOwnedByTarget = menu->currentItems == &menu->rightItems;
		int actionPointCost = CalculateCombatPickpocketAPCost(selection, actor, itemValue, count, isItemOwnedByTarget);
		int playerActionPoints = g_playerAVOwner->GetActorValue(kAVCode_ActionPoints);
		if (actionPointCost > playerActionPoints)
		{
			ThisStdCall(0x8C00E0, g_thePlayer, actor, 0, 0);
			char buf[260];
			sprintf(buf, "You don't have enough Action Points to steal this item.");
			QueueUIMessage(buf, eEmotion::sad, NULL, NULL, 2.0, 0);

			//menu->hasFailedPickpocket= true; //not sure what the point of this is.
			wasSuccessful = false;
		}
		else
		{
			g_thePlayer->DamageActionPoints(actionPointCost);

			/*
			if (menu->currentItems == &menu->rightItems && !menu->hasPickedPocket)
			{
				IncPCMiscStat(kMiscStat_PocketsPicked);
				menu->hasPickedPocket = true;

				if (g_bPickpocketRewardXP)
				{
					float itemWeight = selection->type->GetWeight() * count;
					int targetPerception = actor->avOwner.GetThresholdedActorValue(kAVCode_Perception);
					g_thePlayer->RewardXP(itemWeight * g_fPickpocketItemWeightMult + (itemValue * g_fPickpocketItemValueMult) + targetPerception);
				}

			}*/
		}

		// decrease the player's karma if they didn't try to steal from someone evil or very evil
		float stolenActorKarma = actor->avOwner.GetActorValue(kAVCode_Karma);
		KarmaTier stolenKarmaTier = GetKarmaTier(stolenActorKarma);
		if (stolenKarmaTier != KarmaTier::Evil && stolenKarmaTier != KarmaTier::VeryEvil)
		{
			int karmaMod = *(float*)0x11CDE24; // fKarmaModStealing
			ThisStdCall(0x94FD30, g_thePlayer, karmaMod);
		}
	}
	return wasSuccessful;
}

void SetContainerSubtitleStringToPickpocketAPCost()
{
	ContainerMenu* container = ContainerMenu::GetSingleton();

	// if not in pickpocket mode return
	if (container->mode != ContainerMenu::Mode::kPickpocket || !IS_TYPE(container->containerRef, Character)) return;

	// get the subtitles tile
	TileMenu* tileMenu = container->tile;

	Tile* subtitlesTile = CdeclCall<Tile*>(0xA08B20, tileMenu, "CM_SUBTITLE");
	if (!subtitlesTile) return;

	bool isStealNotPlace = (container->currentItems == &container->rightItems);
	int itemValue = isStealNotPlace ? ThisStdCall<int>(0x75E240, container, ContainerMenu::GetSelection(), 1, 1) : 1;

	bool isItemOwnedByTarget = container->currentItems == &container->rightItems;
	int actionPointCost = CalculateCombatPickpocketAPCost(ContainerMenu::GetSelection(), (Actor*)container->containerRef, itemValue, 1, isItemOwnedByTarget);
	int currentAP = g_playerAVOwner->GetActorValue(kAVCode_ActionPoints);
	char buf[260];
	sprintf(buf, "Action Point Cost: %d / %d", actionPointCost, currentAP);
	subtitlesTile->SetString(kTileValue_string, buf, 1);
}

__declspec(naked) void ContainerHoverItemHook()
{
	static const UInt32 retnAddr = 0x75CEDA;
	_asm
	{
		call	SetContainerSubtitleStringToPickpocketAPCost

		originalCode :
			mov		ecx, [ebp - 4]
			mov		edx, [ecx + 0x5C]
			jmp		retnAddr
	}
}

bool IsPickpocketHookSet = false;
UINT32 PickpocketFuncAddr1 = 0;
UINT32 PickpocketFuncAddr2 = 0;
UINT32 PickpocketFuncAddr3 = 0;

void resetPickpocketHooks()
{	
	//Undo function hooks.
	WriteRelCall(0x75DBDA, UINT32(PickpocketFuncAddr1));
	WriteRelCall(0x75DFA7, UINT32(PickpocketFuncAddr2));
	WriteRelJump(0x75CED4, UINT32(PickpocketFuncAddr3));
	SafeWrite8(0x5FA8E4, 0x74);
	SafeWrite8(0x608200, 0x74);

	IsPickpocketHookSet = false;
	Console_Print("Changes offline.");
} 


//Replaces a IsInCombat check in the NPC activation code.
bool __fastcall PCCanPickpocketInCombatHOOK(Actor* actor, void* edx)
{
	bool const isInCombat = ThisStdCall<bool>(0x493BB0, actor);
	if (isInCombat)
	{
		if (g_canPlayerPickpocketInCombat && g_thePlayer->IsSneaking())
		{
			if (!IsPickpocketHookSet)
			{				
				//Replace some function calls, store the OPCodes of the functions we're replacing, set a flag to prevent repeating work.
				IsPickpocketHookSet = true;
				PickpocketFuncAddr1 = GetRelJumpAddr(0x75DBDA);
				PickpocketFuncAddr2 = GetRelJumpAddr(0x75DFA7);
				PickpocketFuncAddr3 = GetRelJumpAddr(0x75CED4);
				//PickpocketFuncAddr4

				//Replace the pickpocket calculation code with an AP cost system.
				WriteRelCall(0x75DBDA, UInt32(TryCombatPickpocket));
				WriteRelCall(0x75DFA7, UInt32(TryCombatPickpocket));

				// Set the pickpocket AP Cost when hovered
				WriteRelJump(0x75CED4, UInt32(ContainerHoverItemHook));

				// Clear the string when unhovered
				SafeWrite32(0x10721C0, UInt32(ContainerMenuHandleMouseoverAlt)); //don't bother changing this back, Stewie does the exact same thing anyways.
																						//Could make this a default hook
				// Jump over the "was player caught by this NPC" check
				SafeWrite8(0x608200, 0xEB);
				SafeWrite8(0x5FA8E4, 0xEB); //for creatures

				Console_Print("Changes online.");

			}
			return false;  // so the pickpocket menu is allowed to open despite the actor being in combat.
		}
		if (!g_canPlayerPickpocketInCombat && IsPickpocketHookSet)
			resetPickpocketHooks();
		return true;
	}
	if (IsPickpocketHookSet)
		resetPickpocketHooks();
	return false;
}

//Replaces a "GetIsCombatTarget w/ the player" check.
bool __fastcall ShowPickpocketStringInCombat(Actor* actor, void* edx, char a2)
{
	bool isInCombat = ThisStdCall<bool>(0x8BC700, actor, g_thePlayer);
	if (isInCombat && g_canPlayerPickpocketInCombat && g_thePlayer->IsSneaking())
	{
		Console_Print("I made a difference!"); //seems like we're failing before this even happens...
		return false;
	}
	return isInCombat;
}

//Still doesn't work even with this...
tList<PlayerCharacter::CompassTarget>* __fastcall ShowPickpocketStringInCombat2(PlayerCharacter* player, void* edx)
{
	if (g_canPlayerPickpocketInCombat)
	{
		//Console_Print("I made a difference 2!"); 
		tList<PlayerCharacter::CompassTarget>* nope = NULL;
		return nope;
	}
	return g_thePlayer->compassTargets;
}



void DoHooks()
{
	//Modify a "IsInCombat" check to allow NPC activation even if they are in combat.
	WriteRelCall(0x607E70, UINT32(PCCanPickpocketInCombatHOOK));
	//Same thing but for Creatures (like Super Mutants).
	WriteRelCall(0x5FA81F, UINT32(PCCanPickpocketInCombatHOOK));

	//NOTE: When failing the AP cost check, it counts as if you were caught by the actor.
	//This could cause really weird behavior with the NPC just taking back everything that was previously stolen,
	//not to mention you would no longer be able to pickpocket them normally after combat.
	//
	//Possible solution: open and hook the companion loot exchange menu instead?

	//Below isn't working currently...
	WriteRelCall(0x77738A, UINT32(ShowPickpocketStringInCombat));
	WriteRelCall(0x7772C9, UINT32(ShowPickpocketStringInCombat2));
	//WriteRelCall(0x770C0D, UINT32(ShowPickpocketStringInCombat2)); //breaks health target UI 
}
