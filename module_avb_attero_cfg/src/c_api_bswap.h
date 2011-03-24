#ifndef __C_API_BSWAP_H__
#define __C_API_BSWAP_H__
/**
 * \brief Put unsigned short integer (big endian) to buffer
 *
 * \param dat pointer to start of short integer
 *
 * \param val value to write
 *
 * \return Pointer to first byte beyond short integer
 */
static inline u_char *c_api_put_u_short_be(
    u_char *dat,
    u_short val
)
{
    dat[1] = val&0xff;
    dat[0] = (val>>8)&0xff;
    return (dat+sizeof(u_short));
}

/**
 * \brief Put unsigned short integer (big endian) to buffer
 *
 * \param dat pointer to start of long integer
 *
 * \param val value to write
 *
 * \return Pointer to first byte beyond long integer
 */
static inline u_char *c_api_put_u_long_be(
    u_char *dat,
    u_long val
)
{
    dat[3] = val&0xff;
    dat[2] = (val>>8)&0xff;
    dat[1] = (val>>16)&0xff;
    dat[0] = (val>>24)&0xff;
    return (dat+sizeof(u_long));
}

/**
 * \brief Put unsigned short integer (little endian) to buffer
 *
 * \param dat pointer to start of short integer
 *
 * \param val value to write
 *
 * \return Pointer to first byte beyond short integer
 */
static inline u_char *c_api_put_u_short_le(
    u_char *dat,
    u_short val
)
{
    dat[0] = val&0xff;
    dat[1] = (val>>8)&0xff;
    return (dat+sizeof(u_short));
}

/**
 * \brief Put unsigned long integer (little endian) to buffer
 *
 * \param dat pointer to start of long integer
 *
 * \param val vlaue to write
 *
 * \return Pointer to first byte beyond long integer
 */
static inline u_char *c_api_put_u_long_le(
    u_char *dat,
    u_long val
)
{
    dat[0] = val&0xff;
    dat[1] = (val>>8)&0xff;
    dat[2] = (val>>16)&0xff;
    dat[3] = (val>>24)&0xff;

    return (dat+sizeof(u_long));
}

/**
 * \brief Get unsigned short integer (big endian)
 *
 * \param dat pointer to start of short integer
 *
 * \param val pointer to variable to return short integer in
 *
 * \return Pointer to first byte beyond short integer
 */
static inline u_char *c_api_get_u_short_be(
    u_char *dat,
    u_short *val
)
{
    *val = (dat[0]<<8)|dat[1];
    return (dat+sizeof(u_short));
}

/**
 * \brief Get unsigned long integer (big endian)
 *
 * \param dat pointer to start of long integer
 *
 * \param val pointer to variable to return long integer in
 *
 * \return Pointer to first byte beyond long integer
 */
static inline u_char *c_api_get_u_long_be(
    u_char *dat,
    u_long *val
)
{
    *val = (dat[0]<<24)|(dat[1]<<16)|(dat[2]<<8)|dat[3];
    return (dat+sizeof(u_long));
}

static inline u_char *c_api_get_i_long_be(
    u_char *dat,
    int *val
)
{
    *val = (dat[0]<<24)|(dat[1]<<16)|(dat[2]<<8)|dat[3];
    return (dat+sizeof(u_long));
}

/**
 * \brief Get unsigned short integer (little endian)
 *
 * \param dat pointer to start of short integer
 *
 * \param val pointer to variable to return short integer in
 *
 * \return Pointer to first byte beyond short integer
 */
static inline u_char *c_api_get_u_short_le(
    u_char *dat,
    u_short *val
)
{
    *val = (dat[1]<<8)|dat[0];
    return (dat+sizeof(u_short));
}

/**
 * \brief Get unsigned long integer (little endian)
 *
 * \param dat pointer to start of long integer
 *
 * \param val pointer to variable to return long integer in
 *
 * \return Pointer to first byte beyond long integer
 */
static inline u_char *c_api_get_u_long_le(
    u_char *dat,
    u_long *val
)
{
    *val = (dat[3]<<24)|(dat[2]<<16)|(dat[1]<<8)|dat[0];
    return (dat+sizeof(u_long));
}

#endif

