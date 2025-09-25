
#include <float.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "util/io/logger.h"
#include "util/UI/pannel_collection.h"
#include "util/data_structure/darray.h"
#include "util/io/serializer_yaml.h"
#include "util/system.h"
#include "imgui_config/imgui_config.h"
#include "render/image.h"

#include "dashboard.h"


// ========================================================================================================================================
// visual novel
// ========================================================================================================================================


// currently contains 104 tags (128 possible)
typedef enum {
GT_ACTION, GT_ADVENTURE, GT_ARTBOOK, GT_CARTOON, GT_COMIC, GT_DOUJINSHI, GT_IMAGESET, GT_MANGA,
GT_MANHUA, GT_MANHWA, GT_WESTERN, GT_ONESHOT, GT_FOURKOMA, GT_SHOUJO, GT_SHOUNEN, GT_JOSEI, 
GT_SEINEN, GT_COMEDY, GT_COOKING, GT_CRIME, GT_CROSS_DRESSING, GT_CULTIVATION, GT_DEATH_GAME,
GT_OP_MC, GT_DEGENERATE_MC, GT_DELINQUENTS, GT_DEMENTIA, GT_DEMONS, GT_DRAMA, GT_FANTASY,
GT_FETISH, GT_GAME, GT_GENDER_BENDER, GT_GENDER_SWAP, GT_GHOST, GT_GYARU, GT_HAREM, GT_HATLEQUIN, 
GT_HISTORY, GT_HORROR, GT_ISEKAI, GT_KIDS, GT_MAGIC, GT_MARTIAL_ARTS, GT_MASTER_SERVANT, GT_MECHS,
GT_MEDICAL, GT_MILF, GT_MILITARY, GT_MONSTER_GIRL, GT_MONSTERS, GT_MUSIC, GT_MYSTERY, GT_NINJA, 
GT_OFFICE_WORKERS, GT_OMEGAVERSE, GT_PARODY, GT_PHILOSOPHICAL, GT_POLICE, GT_POST_APOCALYPTIC,
GT_PSYCHOLOGICAL, GT_REINCARNATION, GT_REVERSE_HAREM, GT_ROMANCE,
} genre_tag_lo;

typedef enum {
GT_SAMURAI, GT_SCHOOL_LIFE, GT_SCI_FI, GT_SHOUJOAI, GT_SHOUNENAI, GT_SHOWBIZ, GT_SLICE_OF_LIFE, GT_SPACE, 
GT_SPORTS, GT_STEPFAMILY, GT_SUPERPOWER, GT_SUPERHERO, GT_SUPERNATURAL, GT_SURVIVAL, GT_TEACHER_STUDENTS,
GT_THRILLER, GT_TIME_TRAVEL, GT_TRAGEDY, GT_VAMPIRES, GT_VILLAINESS, GT_VIRTUAL_REALITY, 
GT_WUXIA, GT_XIANXIA, GT_XUANHUAN, GT_ZOMBIES, 

// NSFW tags
GT_GORE, GT_BLOODY, GT_VIOLENCE, GT_ADULT, GT_MATURE, GT_SMUT, GT_ECCHI, GT_NTR, GT_INCEST, 
GT_LOLI, GT_SHOTA, GT_FUTA, GT_BARA, GT_YAOI, GT_YURI
} genre_tag_hi;

const char* genre_tag_lo_to_str(const genre_tag_lo type) {

    switch (type) {
        case GT_ACTION:                 return "action";
        case GT_ADVENTURE:              return "adventure";
        case GT_ARTBOOK:                return "artbook";
        case GT_CARTOON:                return "cartoon";
        case GT_COMIC:                  return "comic";
        case GT_DOUJINSHI:              return "doujinshi";
        case GT_IMAGESET:               return "imageset";
        case GT_MANGA:                  return "manga";
        case GT_MANHUA:                 return "manhua";
        case GT_MANHWA:                 return "manhwa";
        case GT_WESTERN:                return "western";
        case GT_ONESHOT:                return "oneshot";
        case GT_FOURKOMA:               return "fourkoma";
        case GT_SHOUJO:                 return "shoujo";
        case GT_SHOUNEN:                return "shounen";
        case GT_JOSEI:                  return "josei";
        case GT_SEINEN:                 return "seinen";
        case GT_COMEDY:                 return "comedy";
        case GT_COOKING:                return "cooking";
        case GT_CRIME:                  return "crime";
        case GT_CROSS_DRESSING:         return "cross dressing";
        case GT_CULTIVATION:            return "cultivation";
        case GT_DEATH_GAME:             return "death game";
        case GT_OP_MC:                  return "op_mc";
        case GT_DEGENERATE_MC:          return "degenerate mc";
        case GT_DELINQUENTS:            return "delinquents";
        case GT_DEMENTIA:               return "dementia";
        case GT_DEMONS:                 return "demons";
        case GT_DRAMA:                  return "drama";
        case GT_FANTASY:                return "fantasy";
        case GT_FETISH:                 return "fetish";
        case GT_GAME:                   return "game";
        case GT_GENDER_BENDER:          return "gender bender";
        case GT_GENDER_SWAP:            return "gender swap";
        case GT_GHOST:                  return "ghost";
        case GT_GYARU:                  return "gyaru";
        case GT_HAREM:                  return "harem";
        case GT_HATLEQUIN:              return "hatlequin";
        case GT_HISTORY:                return "history";
        case GT_HORROR:                 return "horror";
        case GT_ISEKAI:                 return "isekai";
        case GT_KIDS:                   return "kids";
        case GT_MAGIC:                  return "magic";
        case GT_MARTIAL_ARTS:           return "martial arts";
        case GT_MASTER_SERVANT:         return "master servant";
        case GT_MECHS:                  return "mechs";
        case GT_MEDICAL:                return "medical";
        case GT_MILF:                   return "milf";
        case GT_MILITARY:               return "military";
        case GT_MONSTER_GIRL:           return "monster girl";
        case GT_MONSTERS:               return "monsters";
        case GT_MUSIC:                  return "music";
        case GT_MYSTERY:                return "mystery";
        case GT_NINJA:                  return "ninja";
        case GT_OFFICE_WORKERS:         return "office workers";
        case GT_OMEGAVERSE:             return "omegaverse";
        case GT_PARODY:                 return "parody";
        case GT_PHILOSOPHICAL:          return "philosophical";
        case GT_POLICE:                 return "police";
        case GT_POST_APOCALYPTIC:       return "post apocalyptic";
        case GT_PSYCHOLOGICAL:          return "psychological";
        case GT_REINCARNATION:          return "reincarnation";
        case GT_REVERSE_HAREM:          return "revers eharem";
        case GT_ROMANCE:                return "romance";
        default:                        return "unknown";
    }
}

const char* genre_tag_hi_to_str(const genre_tag_hi type) {

    switch (type) {
        case GT_SAMURAI:                return "samurai";
        case GT_SCHOOL_LIFE:            return "school life";
        case GT_SCI_FI:                 return "sci-fi";
        case GT_SHOUJOAI:               return "shoujoai";
        case GT_SHOUNENAI:              return "shounenai";
        case GT_SHOWBIZ:                return "showbiz";
        case GT_SLICE_OF_LIFE:          return "slice of life";
        case GT_SPACE:                  return "space";
        case GT_SPORTS:                 return "sports";
        case GT_STEPFAMILY:             return "stepfamily";
        case GT_SUPERPOWER:             return "superpower";
        case GT_SUPERHERO:              return "superhero";
        case GT_SUPERNATURAL:           return "supernatural";
        case GT_SURVIVAL:               return "survival";
        case GT_TEACHER_STUDENTS:       return "teacher students";
        case GT_THRILLER:               return "thriller";
        case GT_TIME_TRAVEL:            return "time travel";
        case GT_TRAGEDY:                return "tragedy";
        case GT_VAMPIRES:               return "vampires";
        case GT_VILLAINESS:             return "villainess";
        case GT_VIRTUAL_REALITY:        return "virtual reality";
        case GT_WUXIA:                  return "wuxia";
        case GT_XIANXIA:                return "xianxia";
        case GT_XUANHUAN:               return "xuanhuan";
        case GT_ZOMBIES:                return "zombies";
        case GT_GORE:                   return "gore";
        case GT_BLOODY:                 return "bloody";
        case GT_VIOLENCE:               return "violence";
        case GT_ADULT:                  return "adult";
        case GT_MATURE:                 return "mature";
        case GT_SMUT:                   return "smut";
        case GT_ECCHI:                  return "ecchi";
        case GT_NTR:                    return "ntr";
        case GT_INCEST:                 return "incest";
        case GT_LOLI:                   return "loli";
        case GT_SHOTA:                  return "shota";
        case GT_FUTA:                   return "futa";
        case GT_BARA:                   return "bara";
        case GT_YAOI:                   return "yaoi";
        case GT_YURI:                   return "yuri";
        default:                        return "unknown";
    }
}

#define GENRE_BIT(tag) (((u64)1) << (tag))

// set a genre
static inline void add_genre_lo(u64 *flags, genre_tag_lo tag)        { *flags |= GENRE_BIT(tag); }
static inline void add_genre_hi(u64 *flags, genre_tag_hi tag)        { *flags |= GENRE_BIT(tag); }

// clear a genre
static inline void remove_genre_lo(u64 *flags, genre_tag_lo tag)     { *flags &= ~GENRE_BIT(tag); }
static inline void remove_genre_hi(u64 *flags, genre_tag_hi tag)     { *flags &= ~GENRE_BIT(tag); }

// test a genre
static inline bool has_genre_lo(u64 flags, genre_tag_lo tag)         { return (flags & GENRE_BIT(tag)) != 0; }
static inline bool has_genre_hi(u64 flags, genre_tag_hi tag)         { return (flags & GENRE_BIT(tag)) != 0; }


typedef enum {
    DR_READ_ALL_CHAPTERS,
    DR_FINISHED,
    DR_GOT_BORED,
    DR_AUTHOR_HIATUS,
    DR_DROPPED_BY_TRANSLATOR,
    DR_POOR_TRANSLATION,
    DR_DECLINE_IN_QUALITY,
} discontinue_reason;

const char* discontinue_reason_to_str(const discontinue_reason type) {

    switch (type) {
        case DR_READ_ALL_CHAPTERS:      { return "read all chapters"; }
        case DR_FINISHED:               { return "finished"; }
        case DR_GOT_BORED:              { return "got bored"; }
        case DR_AUTHOR_HIATUS:          { return "author hiatus"; }
        case DR_DROPPED_BY_TRANSLATOR:  { return "dropped by translator"; }
        case DR_POOR_TRANSLATION:       { return "poor translation"; }
        case DR_DECLINE_IN_QUALITY:     { return "decline in quality"; }
        default:                        { return "unknown"; }
    }
}


typedef struct {
    char                name[512];
    char                link[PATH_MAX];
    char                image_path[PATH_MAX];
    u16                 chapters_total;
    u16                 chapters_read;
    u8                  rating;                          // from 0 to 10
    discontinue_reason  disc_reason;
    u64                 flags_lo;                       // enum values from 65-128
    u64                 flags_hi;                       // enum values from 65-128
} visual_novel;

static darray visual_novels = {0};


bool visual_novels_serializer_cb(SY* serializer, void* element) {

    visual_novel* vs = (visual_novel*)element;
    sy_entry_str(serializer, S_KEY_VALUE(*vs->name), sizeof(vs->name));
    sy_entry_str(serializer, S_KEY_VALUE(*vs->link), sizeof(vs->link));
    sy_entry_str(serializer, S_KEY_VALUE(*vs->image_path), sizeof(vs->image_path));
    sy_entry(serializer, S_KEY_VALUE_FORMAT(vs->chapters_total));
    sy_entry(serializer, S_KEY_VALUE_FORMAT(vs->chapters_read));
    sy_entry(serializer, S_KEY_VALUE_FORMAT(vs->rating));
    sy_entry(serializer, S_KEY_VALUE_FORMAT(vs->disc_reason));
    sy_entry(serializer, S_KEY_VALUE_FORMAT(vs->flags_lo));
    sy_entry(serializer, S_KEY_VALUE_FORMAT(vs->flags_hi));
    return true;
}

// ========================================================================================================================================
// dashboard
// ========================================================================================================================================

//
b8 dashboard_init() {

    darray_init(&visual_novels, sizeof(visual_novels));

    char exec_path[PATH_MAX] = {0};
    get_executable_path_buf(exec_path, sizeof(exec_path));
    char loc_file_path[PATH_MAX] = {0};
    memset(loc_file_path, '\0', sizeof(loc_file_path));
    const int written = snprintf(loc_file_path, sizeof(loc_file_path), "%s/%s", exec_path, "config");
    VALIDATE(written >= 0 && (size_t)written < sizeof(loc_file_path), return false, "", "Path too long: %s/%s\n", exec_path, "config");

    SY sy = {0};
    VALIDATE(sy_init(&sy, loc_file_path, "project_data.yml", "general_data", SERIALIZER_OPTION_LOAD), return false, "", "Failed to load project data");
    
#if 0       // use dummy values
    sy_loop(&sy, S_KEY_VALUE(visual_novels), sizeof(visual_novel), visual_novels_serializer_cb, 
        (sy_loop_callback_at_t)darray_get,
        (sy_loop_callback_append_t)darray_push_back,
        (sy_loop_DS_size_callback_t)darray_size);
#else
    // Create some dummy visual novels
    visual_novel vn0 = {
        .name = "Eternal Sakura",
        .link = "https://example.com/eternal-sakura",
        .image_path = "/eternal_sakura.jpg",
        .chapters_total = 24,
        .chapters_read = 24,
        .rating = 9,
        .disc_reason = DR_FINISHED,
        .flags_lo = (1ULL << GT_ROMANCE) | (1ULL << GT_SCHOOL_LIFE) | (1ULL << GT_DRAMA),
        .flags_hi = (1ULL << GT_YURI)  // Assuming 64 is the cutoff between flags_lo and flags_hi
    };

    visual_novel vn1 = {
        .name = "Cyber Nexus Reborn",
        .link = "https://example.com/cyber-nexus",
        .image_path = "/cyber_nexus.png",
        .chapters_total = 48,
        .chapters_read = 15,
        .rating = 7,
        .disc_reason = DR_GOT_BORED,
        .flags_lo = (1ULL << GT_SCI_FI) | (1ULL << GT_ACTION) | (1ULL << GT_PSYCHOLOGICAL),
        .flags_hi = (1ULL << GT_GORE) | (1ULL << GT_VIOLENCE)
    };

    visual_novel vn2 = {
        .name = "Crimson Moon Chronicles",
        .link = "https://example.com/crimson-moon",
        .image_path = "/crimson_moon.jpg",
        .chapters_total = 36,
        .chapters_read = 36,
        .rating = 10,
        .disc_reason = DR_FINISHED,
        .flags_lo = (1ULL << GT_FANTASY) | (1ULL << GT_SUPERNATURAL) | (1ULL << GT_VAMPIRES) | (1ULL << GT_ROMANCE),
        .flags_hi = (1ULL << GT_ADULT) | (1ULL << GT_MATURE)
    };

    visual_novel vn3 = {
        .name = "Starlight Academy",
        .link = "https://example.com/starlight-academy",
        .image_path = "/starlight_academy.png",
        .chapters_total = 18,
        .chapters_read = 5,
        .rating = 6,
        .disc_reason = DR_POOR_TRANSLATION,
        .flags_lo = (1ULL << GT_SCHOOL_LIFE) | (1ULL << GT_COMEDY) | (1ULL << GT_SLICE_OF_LIFE) | (1ULL << GT_HAREM),
        .flags_hi = (1ULL << GT_ECCHI)
    };

    // Add them to the array
    darray_push_back(&visual_novels, &vn0);
    darray_push_back(&visual_novels, &vn1);
    darray_push_back(&visual_novels, &vn2);
    darray_push_back(&visual_novels, &vn3);
#endif

    sy_shutdown(&sy);

    // sleep(3);
    return true;
}

//
void dashboard_shutdown() {

    darray_free(&visual_novels);
    LOG_SHUTDOWN
}

//
void dashboard_on_crash() { LOG(Debug, "User crash_callback")}

//
void dashboard_update(__attribute_maybe_unused__ const f32 delta_time) { }


void draw_card(visual_novel* vn) {
    // Card background
    igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (ImVec2){8, 8});
    igPushStyleVar_Float(ImGuiStyleVar_FrameRounding, 8.0f);
    igPushStyleColor_U32(ImGuiCol_ChildBg, igColorConvertFloat4ToU32((ImVec4){0.15f, 0.15f, 0.15f, 1.0f}));
    igPushStyleColor_U32(ImGuiCol_Border, igColorConvertFloat4ToU32((ImVec4){0.3f, 0.3f, 0.3f, 1.0f}));
    igPushStyleVar_Float(ImGuiStyleVar_ChildBorderSize, 1.0f);
    
    // Calculate card size
    float card_width = 300.0f;
    float card_height = 300.0f;
    
    igBeginChild_Str(vn->name, (ImVec2){card_width, card_height}, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        // Card header with title
        igPushFont(imgui_config_get_font(FT_HEADER_2), g_font_size_header_2);
        igTextWrapped("%s", vn->name);
        igPopFont();
        
        igSeparator();
        
        // Progress information
        float progress = (vn->chapters_total > 0) ? (float)vn->chapters_read / (float)vn->chapters_total : 0.0f;
        igText("Progress: %d/%d chapters", vn->chapters_read, vn->chapters_total);
        igProgressBar(progress, (ImVec2){-FLT_MIN, 0}, NULL);
        
        // Rating (as stars)
        igText("Rating: %d", vn->rating);
        
        // Status
        igText("Status: %s", discontinue_reason_to_str(vn->disc_reason));
        
        // Genre tags (show first 3-4 tags)
        igText("Tags: ");
        igSameLine(0, 4);
        

        // bool line_full = false;
        // for (u8 x = 0; x < 64; x++) {

        //     if ( (vn->flags_lo >> x) & 1 ) {
    
        //         const char* genre_tag_lo_to_str((genre_tag_lo)vn->flags_lo);
        //         igText("...");
        //         // line_full = ;
        //     }

        //     if (line_full)
        //         break;

        // }

        
        #undef CHECK_AND_SHOW_TAG
        
        if (igButton("Open Link", (ImVec2){-FLT_MIN, 0})) {
            LOG(Info, "Opening link: %s", vn->link);
        }
    }
    igEndChild();
    
    igPopStyleVar(3);
    igPopStyleColor(2);
}

//
void dashboard_draw(__attribute_maybe_unused__ const f32 delta_time) {

    // Get main viewport and set main window to cover it
    const ImVec2 pivot = {0};
    ImGuiViewport* main_vp = igGetMainViewport();
    igSetNextWindowPos(main_vp->Pos, ImGuiCond_Always, pivot);
    igSetNextWindowSize(main_vp->Size, ImGuiCond_Always);
    igSetNextWindowViewport(main_vp->ID);

    // Begin main window with no decoration and no padding
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoSavedSettings;
    igBegin("##main_viewport", NULL, window_flags);
    
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});           // Remove window padding for precise layout control
    
    const float sidebar_width = 100.0f;
    igBeginChild_Str("##sidebar", (ImVec2){sidebar_width, 0}, false, 0);        // Left sidebar (fixed width)
    {
        // Restore padding for sidebar content
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){4, 4});
        igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){4, 4});
        
        // Sidebar content (example buttons)
        if (igButton("Settings", (ImVec2){-FLT_MIN, 0})) {}
        if (igButton("Stats", (ImVec2){-FLT_MIN, 0})) {}
        
        igPopStyleVar(2);
    }
    igEndChild();
    
    // Same line for right panel
    igSameLine(0, 0);
    igSetCursorPosX(igGetCursorPosX() + 10);
    

    igBeginChild_Str("##content", (ImVec2){0, 0}, false, 0);
    {
        igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){16, 16});
        igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){16, 16});
        
        // Header
        igPushFont(imgui_config_get_font(FT_GIANT), g_font_size_giant);
        igText("Visual Novels Collection");
        igPopFont();
        
        igSeparator();
        igSpacing();

    #if 0
        // Cards grid
        size_t novel_count = darray_size(&visual_novels);
        if (novel_count > 0) {
            // Calculate how many cards per row based on available width
            float available_width = igGetWindowWidth();
            float card_width = 300.0f;
            float spacing = 16.0f;
            int cards_per_row = (int)(available_width / (card_width + spacing));
            if (cards_per_row < 1) cards_per_row = 1;
            
            int current_card = 0;
            for (size_t i = 0; i < novel_count; i++) {
                visual_novel vn = {0};
                darray_get(&visual_novels, i, &vn);
                    if (current_card > 0 && current_card % cards_per_row != 0) {
                        igSameLine(0, spacing);
                    }
                    draw_card(&vn);
                    current_card++;
            }
        } else
            igText("No visual novels added yet.");
    #else
        
        // TODO: display a test image at [<get_executable_path()>/images/test_image.png]

        ImVec2 image_size = {80, 120};
        static image_t test_image = {0};
        if (test_image.magic == 0) {

            char exe_path[1024] = {0};
            get_executable_path_buf(exe_path, sizeof(exe_path));
            char image_path[2048] = {0};
            snprintf(image_path, sizeof(image_path), "%s/assets/images/test_image.png", exe_path);
            LOG(Trace, "Image at [%s]", image_path)
            VALIDATE(image_create_from_file(&test_image, image_path, IF_RGBA, false), , "", "Failed to create image");
        }
        
        igImage(image_get_texture_id(&test_image), image_size, (ImVec2){0,0}, (ImVec2){1,1});


    #endif
        
        igPopStyleVar(2);
    }
    igEndChild();
    
    igPopStyleVar(1); // Restore WindowPadding
    igEnd(); // End main window
}


void dashboard_draw_init_UI(__attribute_maybe_unused__ const f32 delta_time) {

    static f32 total_time = 0.f;
    total_time += delta_time;
    if (total_time > 1.f)
        total_time = 0.f;
    
    ImGuiViewport* viewport = igGetMainViewport();
    
    // Set window to cover the entire viewport
    const ImVec2 pos_vec = {0};
    igSetNextWindowPos(viewport->WorkPos, ImGuiCond_Always, pos_vec);
    igSetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, pos_vec);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    igBegin("Initialization", NULL, window_flags);
    {
        // Center content in the window
        ImVec2 center_pos = {0};
        center_pos.x = (viewport->WorkSize.x - 200) * 0.5f;
        center_pos.y = (viewport->WorkSize.y - 100) * 0.5f;
        igSetCursorPos(center_pos);
        
        igPushFont(imgui_config_get_font(FT_GIANT), g_font_size_giant);
        if (total_time < (1.f/4.f))
            igText("Initializing");
        else if (total_time < (2.f/4.f))
            igText("Initializing.");
        else if (total_time < (3.f/4.f))
            igText("Initializing..");
        else
            igText("Initializing...");
        igPopFont();

    #if 0   // currently not working, dont know why

        ImVec4 main_color = {1.f, 1.f, 1.f, 1.0f};
        ImVec4 backdrop_color = {0.2f, 0.2f, 0.2f, 1.0f};
        UI_loading_indicator_circle("##loading_indicator", 30, 13, 5, &main_color, &backdrop_color);
    #endif

    }
    igEnd();
    
    igPopStyleVar(2);
}

