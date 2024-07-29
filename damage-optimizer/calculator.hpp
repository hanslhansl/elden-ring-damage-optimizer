#pragma once

#include "include.hpp"



namespace calculator
{
	template<class Key, class T>
	using map = std::map<Key, T>;
	using floating = double;

	BETTER_ENUM(Attribute, int,
		STRENGTH,
		DEXTERITY,
		INTELLIGENCE,
		FAITH,
		ARCAINE
	);
	constexpr const char* attribute_to_json_string(Attribute at);
	constexpr auto attribute_json_string_table = better_enums::make_map(attribute_to_json_string);
	using Stats = std::array<int, Attribute::_size()>;

	BETTER_ENUM(Class, int,
		HERO,
		BANDIT,
		ASTROLOGER,
		WARRIOR,
		PRISONER,
		CONFESSOR,
		WRETCH,
		VAGABOND,
		PROPHET,
		SAMURAI
	);
	const map<Class, Stats> ALL_CLASS_STATS{
		{Class::HERO,		{ 16, 9,  7,  8, 11}},
		{Class::BANDIT,		{ 9, 13,  9,  8, 14}},
		{Class::ASTROLOGER,	{ 8, 12, 16,  7,  9}},
		{Class::WARRIOR,	{10, 16, 10,  8,  9}},
		{Class::PRISONER,	{11, 14, 14,  6,  9}},
		{Class::CONFESSOR,	{12, 12,  9, 14,  9}},
		{Class::WRETCH,		{10, 10, 10, 10, 10}},
		{Class::VAGABOND,	{14, 13,  9,  9,  7}},
		{Class::PROPHET,	{11, 10,  7, 16, 10}},
		{Class::SAMURAI,	{12, 15,  9,  8,  8}}
	};

	BETTER_ENUM(AttackPowerType, int,
		PHYSICAL = 0,
		MAGIC = 1,
		FIRE = 2,
		LIGHTNING = 3,
		HOLY = 4,
		POISON = 5,
		SCARLET_ROT = 6,
		BLEED = 7,
		FROST = 8,
		SLEEP = 9,
		MADNESS = 10,
		DEATH_BLIGHT = 11
	);

	BETTER_ENUM(DamageTypes, int,
		PHYSICAL = AttackPowerType::PHYSICAL,
		MAGIC = AttackPowerType::MAGIC,
		FIRE = AttackPowerType::FIRE,
		LIGHTNING = AttackPowerType::LIGHTNING,
		HOLY = AttackPowerType::HOLY
	);
	BETTER_ENUM(StatusTypes, int,
		POISON = AttackPowerType::POISON,
		SCARLET_ROT = AttackPowerType::SCARLET_ROT,
		BLEED = AttackPowerType::BLEED,
		FROST = AttackPowerType::FROST,
		SLEEP = AttackPowerType::SLEEP,
		MADNESS = AttackPowerType::MADNESS,
		DEATH_BLIGHT = AttackPowerType::DEATH_BLIGHT
	);

	using ScalingCurve = std::array<floating, 149>;
	using AttributeScaling = std::array<floating, Attribute::_size()>;
	using AttackElementCorrects = std::array<AttributeScaling, AttackPowerType::_size()>;
	using AttackElementCorrectsById = map<int, AttackElementCorrects>;

	BETTER_ENUM(Affinity_, int,
		STANDARD = 0,
		HEAVY = 1,
		KEEN = 2,
		QUALITY = 3,
		MAGIC = 4,
		FIRE = 5,
		FLAME_ART = 6,
		LIGHTNING = 7,
		SACRED = 8,
		COLD = 9,
		POISON = 10,
		BLOOD = 11,
		OCCULT = 12,
		UNIQUE = -1
	);

	BETTER_ENUM(Type_, int,
		DAGGER = 1,
		STRAIGHT_SWORD = 3,
		GREATSWORD = 5,
		COLOSSAL_SWORD = 7,
		CURVED_SWORD = 9,
		CURVED_GREATSWORD = 11,
		KATANA = 13,
		TWINBLADE = 14,
		THRUSTING_SWORD = 15,
		HEAVY_THRUSTING_SWORD = 16,
		AXE = 17,
		GREATAXE = 19,
		HAMMER = 21,
		GREAT_HAMMER = 23,
		FLAIL = 24,
		SPEAR = 25,
		GREAT_SPEAR = 28,
		HALBERD = 29,
		REAPER = 31,
		FIST = 35,
		CLAW = 37,
		WHIP = 39,
		COLOSSAL_WEAPON = 41,
		LIGHT_BOW = 50,
		BOW = 51,
		GREATBOW = 53,
		CROSSBOW = 55,
		BALLISTA = 56,
		GLINTSTONE_STAFF = 57,
		DUAL_CATALYST = 59,
		SACRED_SEAL = 61,
		SMALL_SHIELD = 65,
		MEDIUM_SHIELD = 67,
		GREATSHIELD = 69,
		TORCH = 87,
		HAND_TO_HAND = 88,
		PERFUME_BOTTLE = 89,
		THRUSTING_SHIELD = 90,
		THROWING_BLADE = 91,
		BACKHAND_BLADE = 92,
		LIGHT_GREATSWORD = 93,
		GREAT_KATANA = 94,
		BEAST_CLAW = 95
	);

	constexpr auto ineffective_attribute_penalty = 0.4;
	constexpr auto defaultDamageCalcCorrectGraphId = 0;
	constexpr auto defaultStatusCalcCorrectGraphId = 6;

	struct Weapon
	{
		using Affinity = Affinity_;
		using Type = Type_;

		struct AttackOptions
		{
			int available_stat_points;
			Stats minimum_stats;
			std::array<size_t, 2> upgrade_level;	// normal, somber
			bool two_handing;
			bool disable_two_handing_attack_power_bonus = false;
		};

		struct AttackRating
		{
			const Weapon* weapon;
			Stats stats;
			std::array<size_t, 2> upgrade_level;
			floating total_attack_power;
			std::array<std::pair<floating, std::array<floating, 2>>, AttackPowerType::_size()> attack_power;	// first = second[0] + second[1]
			std::array<floating, AttackPowerType::_size()> spell_scaling;
			std::vector<Attribute> ineffective_attributes;
			std::vector<AttackPowerType> ineffective_attack_power_types;

			void reset();
		};

		struct Filter
		{
			std::set<std::string> full_names;
			std::set<std::string> base_names;
			std::set<bool> dlc;
			std::set<bool> sorcery_tools;
			std::set<bool> incantation_tools;
			std::set<Type> types;
			std::set<Affinity> affinities;

			bool operator()(const Weapon& weapon) const;
		};

		// the full unique name of the weapon, e.g. "Heavy Nightrider Glaive"
		const std::string full_name;
		// the base weapon name without an affinity specified, e.g. "Nightrider Glaive"
		const std::string base_name;
		// a wiki link for the weapon
		const std::string url;
		// true if the weapon was introduced with SOTE
		const bool dlc;
		// true if the weapon doesn't get a strength bonus when two-handing
		const bool paired;
		// true if this weapon can cast glintstone sorceries
		const bool sorcery_tool;
		// true if this weapon can cast incantations
		const bool incantation_tool;
		// the category of the weapon, e.g. Type.STRAIGHT_SWORD
		const Type type;
		// the affinity of the weapon, e.g. Affinity.HEAVY
		const Affinity affinity;
		// stat requirements necessary to use the weapon effectively (without an attack rating penalty)
		const Stats requirements;
		// scaling amount at each upgrade level (0-10 or 0-25) for each player attribute (e.g. Attribute.STRENGTH)
		const std::vector<AttributeScaling> attribute_scaling;
		// base attack power at each upgrade level for each attack power type
		const std::vector<std::array<floating, AttackPowerType::_size()>> base_attack_power;
		// map indicating which attack power types scale with which player attributes
		const AttackElementCorrectsById::mapped_type& attack_power_attribute_scaling;
		// map indicating which scaling curve is used for each attack power type
		const std::array<const ScalingCurve*, AttackPowerType::_size()> attack_power_scaling_curves;
		// thresholds and labels for each scaling grade (S, A, B, etc.) for this weapon. This isn't hardcoded for all weapons because it can be changed by mods.
		const std::array<std::pair<floating, std::string>, 6>& scaling_tiers;

		Stats adjust_stats_for_two_handing(bool two_handing, Stats stats) const;

		void get_attack_rating(const AttackOptions& attack_options, const Stats& stats, AttackRating& result) const;
	};

	struct CalcCorrectGraphDict
	{
		int maxVal;
		floating maxGrowVal, adjPt;

		friend void from_json(const json& j, CalcCorrectGraphDict& c);
	};
	struct ReinforceTypesDict
	{
		AttributeScaling attack;	// index: AttackPowerType (if in ALL_DAMAGE_TYPES)
		AttributeScaling attributeScaling;	// index: Attribute
		std::array<int, 3> statusSpEffectId; // statusSpEffectId1, statusSpEffectId2, statusSpEffectId3

		friend void from_json(const json& j, ReinforceTypesDict& r);
	};

	std::vector<Stats> get_stat_variations(const size_t attribute_points, const Stats LOWER);

	template<typename Proj>
	struct optimization_context
	{
		Proj proj;
		std::vector<Weapon::AttackRating> optional_results;
		std::unique_ptr<BS::thread_pool> pool;

		Weapon::AttackRating wait_and_get_result() const
		{
			// wait for all threads to finish
			this->pool->wait();
			misc::printl();

			// loop through all optional attack ratings and get the best one
			Weapon::AttackRating result{};
			for (auto&& optional_result : optional_results)
				if (optional_result.weapon != nullptr and (result.weapon == nullptr or this->proj(result) < this->proj(optional_result)))
					result = std::move(optional_result);

			return result;
		}
	};

	struct filtered_weapons
	{
		std::vector<const Weapon*> weapons;

		template<typename Proj>
			requires requires (const Weapon& w, Proj&& proj, const Weapon::AttackRating& war) { proj(war) < proj(war); }
		auto optimize_attack_rating(const std::vector<Stats>& stat_variations, Proj&& proj, const Weapon::AttackOptions& attack_options) const
		{
			// print some stats
			auto total_combinations = this->weapons.size() * stat_variations.size();
			misc::printl(total_combinations, " variations total\n");

			// create an optimization context
			optimization_context<Proj> context{ std::move(proj), {}, nullptr };
			context.optional_results.resize(this->weapons.size());

			// process one weapon
			auto do_weapon = [&](size_t i)
				{
					const Weapon& weapon = *this->weapons[i];
					auto& best_attack_rating = context.optional_results[i];

					Weapon::AttackRating intermediate_attack_rating{};
					intermediate_attack_rating.ineffective_attack_power_types.reserve(AttackPowerType::_size());
					intermediate_attack_rating.ineffective_attributes.reserve(Attribute::_size());
					decltype(context.proj(intermediate_attack_rating)) best_attack_rating_proj_result{};

					// loop through all stat variations and find the one resulting in the best attack rating
					for (auto&& stats : stat_variations)
					{
						weapon.get_attack_rating(attack_options, stats, intermediate_attack_rating);
						auto intermediate_attack_rating_proj_result = context.proj(intermediate_attack_rating);

						if (best_attack_rating.weapon == nullptr or best_attack_rating_proj_result < intermediate_attack_rating_proj_result)
						{
							best_attack_rating_proj_result = std::move(intermediate_attack_rating_proj_result);
							best_attack_rating = std::move(intermediate_attack_rating);
						}

						intermediate_attack_rating.reset();

						//break;
					}
					misc::print("completed " + std::to_string(i) + ": " + weapon.full_name + "\n");
				};

			// create a thread pool
			context.pool = std::make_unique<BS::thread_pool>(20);

			// loop through all weapons and get the best attack rating each asynchronously
			context.pool->detach_sequence(0ull, this->weapons.size(), do_weapon);

			return context;
		}
	};

	class weapon_container
	{
		using WeaponDict = json;

		map<int, ScalingCurve> calcCorrectGraphsById{};
		AttackElementCorrectsById attackElementCorrectsById{};
		std::array<std::pair<floating, std::string>, 6> scalingTiers{};

		static ScalingCurve evaluate_CalcCorrectGraph(const std::vector<CalcCorrectGraphDict>& calcCorrectGraph);
	public:
		std::vector<Weapon> weapons{};

		weapon_container(const std::filesystem::path& file_path);

		filtered_weapons filter(const Weapon::Filter& weapon_filter) const;
	};

	void test();
}

namespace nlohmann
{
	template<typename T>
	struct adl_serializer<calculator::map<calculator::AttackPowerType, T>>
	{
		static void from_json(const json& j, calculator::map<calculator::AttackPowerType, T>& m)
		{
			for (const auto& [apt_str, val] : j.items())
			{
				auto apt = calculator::AttackPowerType::_from_integral(std::stoi(apt_str));

				m[apt] = val.get<T>();
			}
		}
	};

	template<>
	struct adl_serializer<calculator::AttributeScaling>
	{
		static void from_json(const json& j, calculator::AttributeScaling& ac);
	};
	template<>
	struct adl_serializer<calculator::AttackElementCorrects>
	{
		static void from_json(const json& j, calculator::AttackElementCorrects& aec);
	};
	template<>
	struct adl_serializer<calculator::Stats>
	{
		static void from_json(const json& j, calculator::Stats& s);
	};
	template<>
	struct adl_serializer<calculator::Attribute>
	{
		static calculator::Attribute from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::AttackPowerType>
	{
		static calculator::AttackPowerType from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::Weapon::Affinity>
	{
		static calculator::Weapon::Affinity from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::Weapon::Type>
	{
		static calculator::Weapon::Type from_json(const json& j);
	};
}

