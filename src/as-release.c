/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014-2015 Matthias Klumpp <matthias@tenstral.net>
 * Copyright (C)      2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the license, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:as-release
 * @short_description: Object representing a single upstream release
 * @include: appstream.h
 *
 * This object represents a single upstream release, typically a minor update.
 * Releases can contain a localized description of paragraph and list elements
 * and also have a version number and timestamp.
 *
 * Releases can be automatically generated by parsing upstream ChangeLogs or
 * .spec files, or can be populated using MetaInfo files.
 *
 * See also: #AsComponent
 */

#include "as-release.h"

typedef struct
{
	gchar		*version;
	GHashTable	*description;
	guint64		timestamp;
	gchar		*active_locale;

	GPtrArray	*locations;
	gchar		**checksums;
	guint64		size[AS_SIZE_KIND_LAST];

	AsUrgencyKind	urgency;
} AsReleasePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AsRelease, as_release, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (as_release_get_instance_private (o))

/**
 * as_checksum_kind_to_string:
 * @kind: the %AsChecksumKind.
 *
 * Converts the enumerated value to an text representation.
 *
 * Returns: string version of @kind
 **/
const gchar*
as_checksum_kind_to_string (AsChecksumKind kind)
{
	if (kind == AS_CHECKSUM_KIND_NONE)
		return "none";
	if (kind == AS_CHECKSUM_KIND_SHA1)
		return "sha1";
	if (kind == AS_CHECKSUM_KIND_SHA256)
		return "sha256";
	return "unknown";
}

/**
 * as_checksum_kind_from_string:
 * @kind_str: the string.
 *
 * Converts the text representation to an enumerated value.
 *
 * Returns: a #AsChecksumKind or %AS_CHECKSUM_KIND_NONE for unknown
 **/
AsChecksumKind
as_checksum_kind_from_string (const gchar *kind_str)
{
	if (g_strcmp0 (kind_str, "none") == 0)
		return AS_CHECKSUM_KIND_NONE;
	if (g_strcmp0 (kind_str, "sha1") == 0)
		return AS_CHECKSUM_KIND_SHA1;
	if (g_strcmp0 (kind_str, "sha256") == 0)
		return AS_CHECKSUM_KIND_SHA256;
	return AS_CHECKSUM_KIND_NONE;
}

/**
 * as_size_kind_to_string:
 * @size_kind: the #AsSizeKind.
 *
 * Converts the enumerated value to an text representation.
 *
 * Returns: string version of @size_kind
 *
 * Since: 0.8.6
 **/
const gchar*
as_size_kind_to_string (AsSizeKind size_kind)
{
	if (size_kind == AS_SIZE_KIND_INSTALLED)
		return "installed";
	if (size_kind == AS_SIZE_KIND_DOWNLOAD)
		return "download";
	return "unknown";
}

/**
 * as_size_kind_from_string:
 * @size_kind: the string.
 *
 * Converts the text representation to an enumerated value.
 *
 * Returns: an #AsSizeKind or %AS_SIZE_KIND_UNKNOWN for unknown
 *
 * Since: 0.8.6
 **/
AsSizeKind
as_size_kind_from_string (const gchar *size_kind)
{
	if (g_strcmp0 (size_kind, "download") == 0)
		return AS_SIZE_KIND_DOWNLOAD;
	if (g_strcmp0 (size_kind, "installed") == 0)
		return AS_SIZE_KIND_INSTALLED;
	return AS_SIZE_KIND_UNKNOWN;
}

/**
 * as_release_init:
 **/
static void
as_release_init (AsRelease *release)
{
	guint i;
	AsReleasePrivate *priv = GET_PRIVATE (release);

	priv->active_locale = g_strdup ("C");
	priv->description = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	priv->locations = g_ptr_array_new_with_free_func (g_free);

	priv->checksums = g_new0 (gchar*, AS_CHECKSUM_KIND_LAST + 1);
	priv->urgency = AS_URGENCY_KIND_UNKNOWN;

	for (i = 0; i < AS_SIZE_KIND_LAST; i++)
		priv->size[i] = 0;
}

/**
 * as_release_finalize:
 **/
static void
as_release_finalize (GObject *object)
{
	AsRelease *release = AS_RELEASE (object);
	AsReleasePrivate *priv = GET_PRIVATE (release);

	g_free (priv->version);
	g_free (priv->active_locale);
	g_hash_table_unref (priv->description);
	g_ptr_array_unref (priv->locations);
	g_strfreev (priv->checksums);

	G_OBJECT_CLASS (as_release_parent_class)->finalize (object);
}

/**
 * as_release_class_init:
 **/
static void
as_release_class_init (AsReleaseClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = as_release_finalize;
}

/**
 * as_release_get_version:
 * @release: a #AsRelease instance.
 *
 * Gets the release version.
 *
 * Returns: string, or %NULL for not set or invalid
 **/
const gchar *
as_release_get_version (AsRelease *release)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	return priv->version;
}

/**
 * as_release_set_version:
 * @release: a #AsRelease instance.
 * @version: the version string.
 *
 * Sets the release version.
 **/
void
as_release_set_version (AsRelease *release, const gchar *version)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	g_free (priv->version);
	priv->version = g_strdup (version);
}

/**
 * as_release_get_timestamp:
 * @release: a #AsRelease instance.
 *
 * Gets the release timestamp.
 *
 * Returns: timestamp, or 0 for unset
 **/
guint64
as_release_get_timestamp (AsRelease *release)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	return priv->timestamp;
}

/**
 * as_release_set_timestamp:
 * @release: a #AsRelease instance.
 * @timestamp: the timestamp value.
 *
 * Sets the release timestamp.
 **/
void
as_release_set_timestamp (AsRelease *release, guint64 timestamp)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	priv->timestamp = timestamp;
}

/**
 * as_release_get_urgency:
 * @release: a #AsRelease instance.
 *
 * Gets the urgency of the release
 * (showing how important it is to update to a more recent release)
 *
 * Returns: #AsUrgencyKind, or %AS_URGENCY_KIND_UNKNOWN for not set
 *
 * Since: 0.6.5
 **/
AsUrgencyKind
as_release_get_urgency (AsRelease *release)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	return priv->urgency;
}

/**
 * as_release_set_urgency:
 * @release: a #AsRelease instance.
 * @urgency: the urgency of this release/update (as #AsUrgencyKind)
 *
 * Sets the release urgency.
 *
 * Since: 0.6.5
 **/
void
as_release_set_urgency (AsRelease *release, AsUrgencyKind urgency)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	priv->urgency = urgency;
}

/**
 * as_release_get_size:
 * @release: a #AsRelease instance
 * @kind: a #AsSizeKind
 *
 * Gets the release size.
 *
 * Returns: The size of the given kind of this release.
 *
 * Since: 0.8.6
 **/
guint64
as_release_get_size (AsRelease *release, AsSizeKind kind)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	g_return_val_if_fail (kind < AS_SIZE_KIND_LAST, 0);
	return priv->size[kind];
}

/**
 * as_release_set_size:
 * @release: a #AsRelease instance
 * @size: a size in bytes, or 0 for unknown
 * @kind: a #AsSizeKind
 *
 * Sets the release size for the given kind.
 *
 * Since: 0.8.6
 **/
void
as_release_set_size (AsRelease *release, guint64 size, AsSizeKind kind)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	g_return_if_fail (kind < AS_SIZE_KIND_LAST);
	g_return_if_fail (kind != 0);

	priv->size[kind] = size;
}

/**
 * as_release_get_description:
 * @release: a #AsRelease instance.
 *
 * Gets the release description markup for a given locale.
 *
 * Returns: markup, or %NULL for not set or invalid
 **/
const gchar*
as_release_get_description (AsRelease *release)
{
	const gchar *desc;
	AsReleasePrivate *priv = GET_PRIVATE (release);

	desc = g_hash_table_lookup (priv->description, priv->active_locale);
	if (desc == NULL) {
		/* fall back to untranslated / default */
		desc = g_hash_table_lookup (priv->description, "C");
	}

	return desc;
}

/**
 * as_release_set_description:
 * @release: a #AsRelease instance.
 * @description: the description markup.
 *
 * Sets the description release markup.
 **/
void
as_release_set_description (AsRelease *release, const gchar *description, const gchar *locale)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);

	if (locale == NULL)
		locale = priv->active_locale;

	g_hash_table_insert (priv->description,
				g_strdup (locale),
				g_strdup (description));
}

/**
 * as_release_get_active_locale:
 *
 * Get the current active locale, which
 * is used to get localized messages.
 */
gchar*
as_release_get_active_locale (AsRelease *release)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	return priv->active_locale;
}

/**
 * as_release_set_active_locale:
 *
 * Set the current active locale, which
 * is used to get localized messages.
 * If the #AsComponent linking this #AsRelease was fetched
 * from a localized database, usually only
 * one locale is available.
 */
void
as_release_set_active_locale (AsRelease *release, const gchar *locale)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);

	g_free (priv->active_locale);
	priv->active_locale = g_strdup (locale);
}

/**
 * as_release_get_locations:
 *
 * Gets the release locations, typically URLs.
 *
 * Returns: (transfer none) (element-type utf8): list of locations
 *
 * Since: 0.8.1
 **/
GPtrArray*
as_release_get_locations (AsRelease *release)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	return priv->locations;
}

/**
 * as_release_add_location:
 * @location: An URL of the download location
 *
 * Adds a release location.
 *
 * Since: 0.8.1
 **/
void
as_release_add_location (AsRelease *release, const gchar *location)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	g_ptr_array_add (priv->locations, g_strdup (location));
}

/**
 * as_release_get_checksum:
 *
 * Gets the release checksum
 *
 * Returns: string, or %NULL for not set or invalid
 *
 * Since: 0.8.2
 **/
const gchar*
as_release_get_checksum (AsRelease *release, AsChecksumKind kind)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	g_return_val_if_fail (kind < AS_CHECKSUM_KIND_LAST, NULL);
	if (kind == 0)
		return NULL;

	return priv->checksums[kind];
}

/**
 * as_release_set_checksum:
 * @release: An instance of #AsRelease.
 * @checksum: The checksum as string.
 * @kind: The kind of this checksum, e.g. %AS_CHECKSUM_KIND_SHA256
 *
 * Set the release checksum.
 *
 * Since: 0.8.2
 */
void
as_release_set_checksum (AsRelease *release, const gchar *checksum, AsChecksumKind kind)
{
	AsReleasePrivate *priv = GET_PRIVATE (release);
	g_return_if_fail (kind < AS_CHECKSUM_KIND_LAST);
	g_return_if_fail (kind != 0);

	g_free (priv->checksums[kind]);
	priv->checksums[kind] = g_strdup (checksum);
}

/**
 * as_release_new:
 *
 * Creates a new #AsRelease.
 *
 * Returns: (transfer full): a #AsRelease
 **/
AsRelease*
as_release_new (void)
{
	AsRelease *release;
	release = g_object_new (AS_TYPE_RELEASE, NULL);
	return AS_RELEASE (release);
}
