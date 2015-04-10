/*
 * Copyright (C) 2009 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "webkitsoupauthdialog.h"

#include "AuthenticationClient.h"
#include "ResourceHandle.h"
#include "webkitauthenticationdialog.h"
#include "webkitmarshal.h"
#include <wtf/text/CString.h>

using namespace WebCore;

/**
 * SECTION:webkitsoupauthdialog
 * @short_description: A #SoupSessionFeature to provide a simple
 * authentication dialog for HTTP basic auth support.
 *
 * #WebKitSoupAuthDialog is a #SoupSessionFeature that you can attach to your
 * #SoupSession to provide a simple authentication dialog while
 * handling HTTP basic auth.
 */


// This class exists only for API compatibility reasons. WebKitSoupAuthDialog was exposed
// in the public API, so we must provide this "fake" AuthenticationClient in order to
// continue using GtkAuthenticationDialog with the new authentication architecture.
class WebKitSoupAuthDialogAuthenticationClient : public WebCore::AuthenticationClient, public RefCounted<WebKitSoupAuthDialogAuthenticationClient> {
using RefCounted<WebKitSoupAuthDialogAuthenticationClient>::ref;
using RefCounted<WebKitSoupAuthDialogAuthenticationClient>::deref;
public:
    virtual void didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
    {
    }

    virtual void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
    {
        soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());
    }

    virtual void receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
    {
        soup_auth_authenticate(challenge.soupAuth(), credential.user().utf8().data(), credential.password().utf8().data());
        soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());
    }

    virtual void receivedCancellation(const AuthenticationChallenge& challenge)
    {
        soup_session_unpause_message(challenge.soupSession(), challenge.soupMessage());
    }

    // This seems necessary to make the compiler happy. Both AuthenticationClient and
    // RefCounted<T> expose a ref/deref method, which interferes with the use of a RefPtr.
    void derefWebKitSoupAuthDialogAuthenticationClient()
    {
        deref();
    }

private:
    virtual void refAuthenticationClient() { ref(); }
    virtual void derefAuthenticationClient() { deref(); }
};

static void webkit_soup_auth_dialog_session_feature_init(SoupSessionFeatureInterface*, gpointer);
static void attach(SoupSessionFeature*, SoupSession*);
static void detach(SoupSessionFeature*, SoupSession*);

enum {
    CURRENT_TOPLEVEL,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE(WebKitSoupAuthDialog, webkit_soup_auth_dialog, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(SOUP_TYPE_SESSION_FEATURE,
                                              webkit_soup_auth_dialog_session_feature_init))

static void webkit_soup_auth_dialog_class_init(WebKitSoupAuthDialogClass* klass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);

    /**
     * WebKitSoupAuthDialog::current-toplevel:
     * @authDialog: the object on which the signal is emitted
     * @message: the #SoupMessage being used in the authentication process
     *
     * This signal is emitted by the @authDialog when it needs to know
     * the current toplevel widget in order to correctly set the
     * transiency for the authentication dialog.
     *
     * Return value: (transfer none): the current toplevel #GtkWidget or %NULL if there's none
     *
     * Since: 1.1.1
     */
    signals[CURRENT_TOPLEVEL] = g_signal_new("current-toplevel",
        G_OBJECT_CLASS_TYPE(objectClass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitSoupAuthDialogClass, current_toplevel),
        0, 0,
        webkit_marshal_OBJECT__OBJECT,
        GTK_TYPE_WIDGET, 1,
        SOUP_TYPE_MESSAGE);
}

static void webkit_soup_auth_dialog_init(WebKitSoupAuthDialog*)
{
}

static void webkit_soup_auth_dialog_session_feature_init(SoupSessionFeatureInterface *featureInterface, gpointer)
{
    featureInterface->attach = attach;
    featureInterface->detach = detach;
}

static void sessionAuthenticate(SoupSession* session, SoupMessage* message, SoupAuth* auth, gboolean retrying, SoupSessionFeature* manager)
{
    GtkWindow* toplevel = 0;
    g_signal_emit(manager, signals[CURRENT_TOPLEVEL], 0, message, &toplevel);

    WebKitSoupAuthDialogAuthenticationClient* client = new WebKitSoupAuthDialogAuthenticationClient();
    AuthenticationChallenge challenge(session, message, auth, retrying, client);
    soup_session_unpause_message(session, message);

    // A RefPtr would be better here, but it seems that accessing RefCounted::deref from this context is
    // impossible with gcc, due to WebKitSoupAuthDialogAuthenticationClient's two superclasses.
    client->derefWebKitSoupAuthDialogAuthenticationClient();

    GtkWidget* authDialog = createAuthenticationDialog(toplevel, challenge, DisallowPersistentStorage);
    gtk_widget_show(authDialog);
}

static void attach(SoupSessionFeature* manager, SoupSession* session)
{
    g_signal_connect(session, "authenticate", G_CALLBACK(sessionAuthenticate), manager);
}

static void detach(SoupSessionFeature* manager, SoupSession* session)
{
    g_signal_handlers_disconnect_by_func(session, reinterpret_cast<gpointer>(sessionAuthenticate), manager);
}


