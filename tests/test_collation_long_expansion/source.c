enum net_device_flags {
    IFF_UP				= 1<<0,  /* sysfs */
    IFF_BROADCAST			= 1<<1,  /* __volatile__ */
    IFF_DEBUG			= 1<<2,  /* sysfs */
    IFF_LOOPBACK			= 1<<3,  /* __volatile__ */
    IFF_POINTOPOINT			= 1<<4,  /* __volatile__ */
    IFF_NOTRAILERS			= 1<<5,  /* sysfs */
    IFF_RUNNING			= 1<<6,  /* __volatile__ */
    IFF_NOARP			= 1<<7,  /* sysfs */
    IFF_PROMISC			= 1<<8,  /* sysfs */
    IFF_ALLMULTI			= 1<<9,  /* sysfs */
    IFF_MASTER			= 1<<10, /* __volatile__ */
    IFF_SLAVE			= 1<<11, /* __volatile__ */
    IFF_MULTICAST			= 1<<12, /* sysfs */
    IFF_PORTSEL			= 1<<13, /* sysfs */
    IFF_AUTOMEDIA			= 1<<14, /* sysfs */
    IFF_DYNAMIC			= 1<<15, /* sysfs */
    IFF_LOWER_UP			= 1<<16, /* __volatile__ */
    IFF_DORMANT			= 1<<17, /* __volatile__ */
    IFF_ECHO			= 1<<18, /* __volatile__ */
};

#define _printf_(a,b) __attribute__ ((format (printf, a, b)))

int log_internal(
                int level,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *format, ...) _printf_(6,7);

int log_object_internal(
                int level,
                int error,
                const char *file,
                int line,
                const char *func,
                const char *object_field,
                const char *object,
                const char *format, ...) _printf_(8,9);

#define log_link_full(link, level, error, ...)                          \
        ({                                                              \
                Link *_l = (link);                                      \
                _l ? log_object_internal(level, error, __FILE__, __LINE__, __func__, "INTERFACE=", _l->ifname, ##__VA_ARGS__) : \
                        log_internal(level, error, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        })                                                              \

#define log_link_debug(link, ...)   log_link_full(link, LOG_DEBUG, 0, ##__VA_ARGS__)


#define FLAG_STRING(string, flag, old, new) \
        (((old ^ new) & flag) \
                ? ((old & flag) ? (" -" string) : (" +" string)) \
                : "")

typedef struct Link {
    int flags;
    char *ifname;
} Link;

#define LOG_DEBUG 3
#define IFF_LOOPBACK IFF_LOOPBACK

void func(Link *link) {
    unsigned flags;

    log_link_debug(link, "Flags change:%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                   FLAG_STRING("LOOPBACK", IFF_LOOPBACK, link->flags, flags),
                   FLAG_STRING("MASTER", IFF_MASTER, link->flags, flags),
                   FLAG_STRING("SLAVE", IFF_SLAVE, link->flags, flags),
                   FLAG_STRING("UP", IFF_UP, link->flags, flags),
                   FLAG_STRING("DORMANT", IFF_DORMANT, link->flags, flags),
                   FLAG_STRING("LOWER_UP", IFF_LOWER_UP, link->flags, flags),
                   FLAG_STRING("RUNNING", IFF_RUNNING, link->flags, flags),
                   FLAG_STRING("MULTICAST", IFF_MULTICAST, link->flags, flags),
                   FLAG_STRING("BROADCAST", IFF_BROADCAST, link->flags, flags),
                   FLAG_STRING("POINTOPOINT", IFF_POINTOPOINT, link->flags, flags),
                   FLAG_STRING("PROMISC", IFF_PROMISC, link->flags, flags),
                   FLAG_STRING("ALLMULTI", IFF_ALLMULTI, link->flags, flags),
                   FLAG_STRING("PORTSEL", IFF_PORTSEL, link->flags, flags),
                   FLAG_STRING("AUTOMEDIA", IFF_AUTOMEDIA, link->flags, flags),
                   FLAG_STRING("DYNAMIC", IFF_DYNAMIC, link->flags, flags),
                   FLAG_STRING("NOARP", IFF_NOARP, link->flags, flags),
                   FLAG_STRING("NOTRAILERS", IFF_NOTRAILERS, link->flags, flags),
                   FLAG_STRING("DEBUG", IFF_DEBUG, link->flags, flags),
                   FLAG_STRING("ECHO", IFF_ECHO, link->flags, flags));
}
