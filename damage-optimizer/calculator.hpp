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

	struct weapon
	{
		using Affinity = Affinity_;
		using Type = Type_;

		struct attack_options
		{
			int available_stat_points;
			Stats minimum_stats;
			std::array<size_t, 2> upgrade_level;	// normal, somber
			bool two_handing;
			bool disable_two_handing_attack_power_bonus = false;
		};

		struct attack_rating
		{
			const weapon* weapon;
			Stats stats;
			std::array<size_t, 2> upgrade_level;
			floating total_attack_power;
			std::array<std::pair<floating, std::array<floating, 2>>, AttackPowerType::_size()> attack_power;	// first = second[0] + second[1]
			std::array<floating, AttackPowerType::_size()> spell_scaling;
			std::vector<Attribute> ineffective_attributes;
			std::vector<AttackPowerType> ineffective_attack_power_types;

			void reset();
		};

		struct filter
		{
			std::set<bool> dlc;
			//std::set<bool> sorcery_tools;
			//std::set<bool> incantation_tools;
			std::set<Type> types;
			std::set<Affinity> affinities;
			std::set<std::string> base_names;

			bool operator()(const weapon& weapon) const;
		};

		struct all_filter_options
		{
			std::vector<bool> dlc;
			//std::vector<bool> sorcery_tools;
			//std::vector<bool> incantation_tools;
			std::vector<Type> types;
			std::vector<Affinity> affinities;
			std::vector<std::string> base_names;
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

		void get_attack_rating(const attack_options& attack_options, const Stats& stats, attack_rating& result) const;
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

	constexpr size_t get_stat_variation_count(const int attribute_points, const Stats& min_stats)
	{
		constexpr auto N = Stats{}.size();
		constexpr auto UPPER = 99;
		const auto SUM = attribute_points;
		size_t count = 0;

		if (attribute_points > UPPER * min_stats.size())
			return 0;
			//throw std::invalid_argument("attribute_points must be <= " + std::to_string(UPPER) + " * " + std::to_string(N));

		if (std::ranges::any_of(min_stats, [](int v) { return v > UPPER; }))
			throw std::invalid_argument("min_stats must be <= " + std::to_string(UPPER));

		for (auto i = min_stats[0]; i <= std::min(UPPER, SUM); ++i)
		{
			auto SUM_i = SUM - i;
			for (auto j = min_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
			{
				auto SUM_i_j = SUM_i - j;

				if (0ll == min_stats[4])
				{
					auto a1 = std::max(min_stats[2], SUM_i_j - min_stats[3] - UPPER);
					auto b1 = std::min(UPPER, SUM_i_j - min_stats[3]);
					auto b1_a1_1 = b1 - a1 + 1;
					if (b1_a1_1 > 0)
						count += (1 - min_stats[3]) * b1_a1_1;

					auto a2 = a1;
					auto b2 = std::min(UPPER, SUM_i_j - UPPER - 1);
					auto b2_a2_1 = b2 - a2 + 1;
					if (b2_a2_1 > 0)
						count += UPPER * b2_a2_1;

					auto a3 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b3 = b1;
					auto b3_a3_1 = b3 - a3 + 1;
					if (b3_a3_1 > 0)
						count += SUM_i_j * b3_a3_1 - (a3 + b3) * b3_a3_1 / 2;

					auto a4 = std::max(min_stats[2], SUM_i_j - UPPER - UPPER);
					auto b4 = std::min(UPPER, SUM_i_j - min_stats[3] - UPPER - 1);
					auto b4_a4_1 = b4 - a4 + 1;
					if (b4_a4_1 > 0)
						count += (UPPER + 1 - SUM_i_j + UPPER) * b4_a4_1 + (a4 + b4) * b4_a4_1 / 2;

					auto a5 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b5 = b4;
					auto b5_a5_1 = b5 - a5 + 1;
					if (b5_a5_1 > 0)
						count += SUM_i_j * b5_a5_1;
				}
				else
				{
					auto a2 = std::max(min_stats[2], SUM_i_j - min_stats[3] - UPPER);
					auto b2 = std::min({ UPPER, SUM_i_j - UPPER - min_stats[4], SUM_i_j - UPPER - 1 });
					auto b2_a2_1 = b2 - a2 + 1;
					if (b2_a2_1 > 0)
						count += (1 + UPPER - min_stats[3]) * b2_a2_1;

					auto a3 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b3 = std::min(UPPER, SUM_i_j - UPPER - min_stats[4]);
					auto b3_a3_1 = b3 - a3 + 1;
					if (b3_a3_1 > 0)
						count += (1 + SUM_i_j - min_stats[3]) * b3_a3_1 - (a3 + b3) * b3_a3_1 / 1;

					auto a4 = std::max({ min_stats[2], SUM_i_j - min_stats[3] - UPPER, SUM_i_j - min_stats[4] - UPPER + 1 });
					auto b4 = std::min(UPPER, SUM_i_j - min_stats[4] - min_stats[3]);
					auto b4_a4_1 = b4 - a4 + 1;
					if (b4_a4_1 > 0)
						count += (SUM_i_j - min_stats[4] - min_stats[3] + 1) * b4_a4_1 - (a4 + b4) * b4_a4_1 / 2;

					auto a5 = std::max(min_stats[2], SUM_i_j - UPPER - UPPER);
					auto b5 = std::min({ UPPER, SUM_i_j - UPPER - 1 - min_stats[3], SUM_i_j - min_stats[4] - UPPER });
					auto b5_a5_1 = b5 - a5 + 1;
					if (b5_a5_1 > 0)
						count += (UPPER - SUM_i_j + UPPER + 1) * b5_a5_1 + (a5 + b5) * b5_a5_1 / 2;

					auto a7 = std::max(min_stats[2], SUM_i_j - UPPER);
					auto b7 = b5;
					auto b7_a7_1 = b7 - a7 + 1;
					if (b7_a7_1 > 0)
						count += (UPPER + 1) * b7_a7_1;

					auto a8 = std::max(min_stats[2], SUM_i_j - UPPER - min_stats[4] + 1);
					auto b8 = std::min(UPPER, SUM_i_j - min_stats[3] - UPPER - 1);
					auto b8_a8_1 = b8 - a8 + 1;
					if (b8_a8_1 > 0)
						count += (UPPER - min_stats[4] + 1) * b8_a8_1;
				}
			}
		}


		return count;
	}
	constexpr std::vector<Stats> get_stat_variations(const int attribute_points, const Stats& min_stats)
	{
		constexpr auto N = Stats{}.size();
		constexpr auto UPPER = 99;
		const auto SUM = attribute_points;

		auto possible_occurances = get_stat_variation_count(attribute_points, min_stats);
		if (possible_occurances == 0)
			return {};
		std::vector<Stats> stat_variations{};
		stat_variations.reserve(possible_occurances);

		for (auto i = min_stats[0]; i <= std::min(UPPER, SUM); ++i)
		{
			auto SUM_i = SUM - i;
			for (auto j = min_stats[1]; j <= std::min(UPPER, SUM_i); ++j)
			{
				auto SUM_i_j = SUM_i - j;
				for (auto k = min_stats[2]; k <= std::min(UPPER, SUM_i_j); ++k)
				{
					auto SUM_i_j_k = SUM_i_j - k;
					for (auto l = min_stats[3]; l <= std::min(UPPER, SUM_i_j_k); ++l)
					{
						auto SUM_i_j_k_l = SUM_i_j_k - l;
						auto m = SUM_i_j_k_l;
						if (min_stats[4] <= m && m <= UPPER)
						{
							Stats indices = { i, j, k, l, m };
							stat_variations.push_back({ i, j, k, l, m });
						}
					}
				}
			}
		}

		return stat_variations;
	}


	template<typename Proj>
	struct optimization_context
	{
		Proj proj;
		std::vector<weapon::attack_rating> optional_results;
		std::unique_ptr<BS::thread_pool> pool;

		weapon::attack_rating wait_and_get_result() const
		{
			// wait for all threads to finish
			this->pool->wait();
			misc::printl();

			// loop through all optional attack ratings and get the best one
			weapon::attack_rating result{};
			for (auto&& optional_result : optional_results)
				if (optional_result.weapon != nullptr and (result.weapon == nullptr or this->proj(result) < this->proj(optional_result)))
					result = std::move(optional_result);

			return result;
		}
	};

	struct filtered_weapons
	{
		std::vector<const weapon*> weapons;

		template<typename Proj>
			requires requires (const weapon& w, Proj&& proj, const weapon::attack_rating& war) { proj(war) < proj(war); }
		auto optimize_attack_rating(const std::vector<Stats>& stat_variations, Proj&& proj, const weapon::attack_options& attack_options) const
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
					const weapon& weapon = *this->weapons[i];
					auto& best_attack_rating = context.optional_results[i];

					weapon::attack_rating intermediate_attack_rating{};
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
		std::vector<weapon> weapons{};

		weapon_container(const std::filesystem::path& file_path);

		weapon::all_filter_options get_all_filter_options() const;

		void apply_filter(const weapon::filter& weapon_filter, filtered_weapons& filtered) const;
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
	struct adl_serializer<calculator::weapon::Affinity>
	{
		static calculator::weapon::Affinity from_json(const json& j);
	};
	template<>
	struct adl_serializer<calculator::weapon::Type>
	{
		static calculator::weapon::Type from_json(const json& j);
	};
}

