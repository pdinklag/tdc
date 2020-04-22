#include <tdc/stat/time.hpp>
#include <tdc/stat/phase.hpp>

#include <sstream>
#include <utility>

using namespace tdc::stat;

const std::string Phase::STAT_TITLE = "title";
const std::string Phase::STAT_TIME = "time";
const std::string Phase::STAT_MEM_OFF = "memOff";
const std::string Phase::STAT_MEM_PEAK = "memPeak";
const std::string Phase::STAT_MEM_FINAL = "memFinal";
const std::string Phase::STAT_NUM_ALLOC = "numAlloc";
const std::string Phase::STAT_NUM_FREE = "numFree";

uint16_t Phase::s_suppress_memory_tracking_state = 0;
uint16_t Phase::s_suppress_tracking_user_state = 0;

bool Phase::s_init = false;
Phase* Phase::s_current = nullptr;
std::vector<std::function<Phase::ext_ptr_t()>> Phase::s_extension_registry;

void Phase::force_malloc_override_link() {
    // Make sure the malloc override is actually linked into the using program.
    //
    // If malloc is never called explicitly, the override won't be linked in
    // a static linking scenario and in that case, memory tracking doesn't work.
    // Thus, here's an explicit call to make sure the link happens.
    //
    // At runtime, this is executed only once when the first StatPhase is
    // initialized.
    void* p = malloc(sizeof(char));
    {
        // make sure all of this isn't "optimized away"
        volatile char* c = (char*)p;
        *c = 0;
    }
    free(p);
}

void Phase::init(std::string&& title) {
    suppress_memory_tracking guard;

    if(!s_init) {
        force_malloc_override_link();
        s_init = true;
    }

    m_parent = s_current;

    m_title = std::move(title);

    // managed allocation of complex members
    m_extensions = std::make_unique<std::vector<ext_ptr_t>>();
    m_sub = std::make_unique<json>(json::array());
    m_stats = std::make_unique<json>();

    // initialize extensions
    for(auto ctor : s_extension_registry) {
        m_extensions->emplace_back(ctor());
    }

    // initialize basic data as the very last thing
    m_num_allocs = 0;
    m_num_frees = 0;
    
    m_mem.off = m_parent ? m_parent->m_mem.current : 0;
    m_mem.current = 0;
    m_mem.peak = 0;

    m_time.end = 0;
    m_time.start = time_millis();
    m_time.paused = 0;

    // set as current
    s_current = this;
}

void Phase::finish() {
    suppress_memory_tracking guard;

    m_time.end = time_millis();

    // let extensions write data
    for(auto& ext : *m_extensions) {
        ext->write(*m_stats);
    }

    if(m_parent) {
        // propagate extensions to parent
        for(size_t i = 0; i < m_extensions->size(); i++) {
            (*(m_parent->m_extensions))[i]->propagate(*(*m_extensions)[i]);
        }

        // add data to parent's data
        m_parent->m_time.paused += m_time.paused;
        m_parent->m_sub->push_back(to_json());
    }

    // managed release of complex members
    m_extensions.reset();
    m_sub.reset();
    m_stats.reset();

    // pop parent
    s_current = m_parent;
}

void Phase::on_pause_tracking() {
    m_pause_time = time_millis();

    // notify extensions
    for(auto& ext : *m_extensions) {
        ext->pause();
    }
}

void Phase::on_resume_tracking() {
    // notify extensions
    for(auto& ext : *m_extensions) {
        ext->resume();
    }

    m_time.paused += time_millis() - m_pause_time;
}

void Phase::track_alloc_internal(size_t bytes) {
    if(is_tracking_memory()) {
        ++m_num_allocs;
        m_mem.current += bytes;
        m_mem.peak = std::max(m_mem.peak, m_mem.current);
        if(m_parent) m_parent->track_alloc_internal(bytes);
    }
}

void Phase::track_free_internal(size_t bytes) {
    if(is_tracking_memory()) {
        ++m_num_frees;
        m_mem.current -= bytes;
        if(m_parent) m_parent->track_free_internal(bytes);
    }
}

Phase::Phase() : m_disabled(true) {
}

Phase::Phase(Phase&& other) {
    *this = std::move(other);
}

Phase::Phase(std::string&& title) {
    init(std::move(title));
}

Phase::~Phase() {
    if (!m_disabled) {
        finish();
    }
}

Phase& Phase::operator=(Phase&& other) {
    m_parent = other.m_parent;
    m_pause_time = other.m_pause_time;
    m_time = other.m_time;
    m_mem = other.m_mem;
    m_num_allocs = other.m_num_allocs;
    m_num_frees = other.m_num_frees;
    m_title = std::move(other.m_title);
    m_sub = std::move(other.m_sub);
    m_stats = std::move(other.m_stats);
    m_disabled = other.m_disabled;
    return *this;
}

void Phase::split(std::string&& new_title) {
    if (!m_disabled) {
        const ssize_t offs = m_mem.off + m_mem.current;
        finish();
        init(std::move(new_title));
        m_mem.off = offs;
    }
}

double Phase::time_run() const {
    return time_millis() - m_time.start - m_time.paused;
}

json Phase::to_json() const {
    const double dt = time_run();
    
    suppress_memory_tracking guard;
    if(!m_disabled) {
        json obj;
        obj[STAT_TITLE] = m_title;
        obj[STAT_TIME] = dt;
        obj[STAT_MEM_OFF] = m_mem.off;
        obj[STAT_MEM_PEAK] = m_mem.peak;
        obj[STAT_MEM_FINAL] = m_mem.current;
        obj[STAT_NUM_ALLOC] = m_num_allocs;
        obj[STAT_NUM_FREE] = m_num_frees;

        // let extensions write data
        for(auto& ext : *m_extensions) {
            ext->write(obj);
        }

        // write user stats
        for(auto it = m_stats->begin(); it != m_stats->end(); it++) {
            obj[it.key()] = *it;
        }

        // sub-phases
        obj["sub"] = *m_sub;
        return obj;
    } else {
        return json();
    }
}

std::string Phase::to_keyval() const {
    const double dt = time_run();
    
    suppress_memory_tracking guard;
    if(!m_disabled) {
        std::ostringstream ss;
        ss << STAT_TIME << "=" << dt - m_time.paused
            << " " << STAT_MEM_OFF << "=" << m_mem.off
            << " " << STAT_MEM_PEAK << "=" << m_mem.peak
            << " " << STAT_MEM_FINAL << "=" << m_mem.current
            << " " << STAT_NUM_ALLOC << "=" << m_num_allocs
            << " " << STAT_NUM_FREE << "=" << m_num_frees;

        // write extension data into temporary json objects
        {
            json obj;
            for(auto& ext : *m_extensions) {
                ext->write(obj);
            }

            // ... and then into result
            for(auto it = obj.begin(); it != obj.end(); it++) {
                ss << " " << it.key() << "=" << *it;
            }
        }

        // finally, write user stats
        for(auto it = m_stats->begin(); it != m_stats->end(); it++) {
            ss << " " << it.key() << "=" << *it;
        }

        return ss.str();
    } else {
        return "";
    }
}

std::string Phase::subphases_keyval(const std::string& value_stat, const std::string& key_stat) const {
    suppress_memory_tracking guard;
    if(!m_disabled) {
        std::ostringstream ss;

        size_t i = 0;
        for(auto& obj : *m_sub) {
            if(i++) ss << " ";
            ss << value_stat << "_" << std::string(obj[key_stat]) << "=" << obj[value_stat];
        }

        return ss.str();
    } else {
        return "";
    }
}
