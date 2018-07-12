#ifndef MACROCOMMAND_H
#define MACROCOMMAND_H

#include "../defs.h"
#include <string>
#include <sstream>
#include <algorithm>

namespace iron
{
	enum class MacroCommandType
	{
		None
		, BarracksRallyNatural
		, KeepTrainSCV
		, KeepTrainMarine
		, KeepBuildBarracks
		, KeepBuildDepot
		, BuildMachineShop
	};

	class MacroCommand
	{
		MacroCommandType	_type;
		int                 _amount;

	public:

		static const list<MacroCommandType> AllCommandTypes()
		{
			return list<MacroCommandType>
			{ MacroCommandType::BarracksRallyNatural
				, MacroCommandType::KeepTrainSCV
				, MacroCommandType::KeepTrainMarine
				, MacroCommandType::KeepBuildBarracks
				, MacroCommandType::KeepBuildDepot
				, MacroCommandType::BuildMachineShop
			};
		}

		// Default constructor for when the value doesn't matter.
		MacroCommand()
			: _type(MacroCommandType::None)
			, _amount(0)
		{
		}

		MacroCommand(MacroCommandType type)
			: _type(type)
			, _amount(0)
		{
		}

		MacroCommand(MacroCommandType type, int amount)
			: _type(type)
			, _amount(amount)
		{
		}

		const int GetAmount() const
		{
			return _amount;
		}

		const MacroCommandType & GetType() const
		{
			return _type;
		}

		// The command has a numeric argument, the _amount.
		static const bool HasArgument(MacroCommandType t)
		{
			return
				t == MacroCommandType::KeepTrainSCV ||
				t == MacroCommandType::KeepTrainMarine ||
				t == MacroCommandType::KeepBuildBarracks ||
				t == MacroCommandType::KeepBuildDepot;
		}

		static const string GetName(MacroCommandType t)
		{
			if (t == MacroCommandType::BarracksRallyNatural)
			{
				return "go barracks rally natural";
			}
			if (t == MacroCommandType::KeepTrainSCV)
			{
				return "go keep train scv";
			}
			if (t == MacroCommandType::KeepTrainMarine)
			{
				return "go keep train marine";
			}
			if (t == MacroCommandType::KeepBuildBarracks)
			{
				return "go keep build barracks";
			}
			if (t == MacroCommandType::KeepBuildDepot)
			{
				return "go keep build supply depot";
			}
			if (t == MacroCommandType::BuildMachineShop)
			{
				return "go build machine shop";
			}

			return "go none";
		}

		const string GetName() const
		{
			if (HasArgument(_type))
			{
				// Include the amount.
				stringstream name;
				name << GetName(_type) << " " << _amount;
				return name.str();
			}
			else {
				return GetName(_type);
			}
		}

	};
}

#endif