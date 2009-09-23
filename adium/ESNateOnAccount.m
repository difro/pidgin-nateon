//
//  ESNateOnAccount.m
//
// code by mario ( xxfree86@gmail.com )

#import <Adium/AIStatusControllerProtocol.h>
#import <Adium/AIAccountControllerProtocol.h>
#import <AIUtilities/AITigerCompatibility.h> 
#import <Adium/ESFileTransfer.h>

#import <AIUtilities/AIMenuAdditions.h>
#import "ESNateOnAccount.h"
#import <Adium/AISharedAdium.h> 

extern void	nateon_act_id_(PurpleConnection *gc, const char *entry);

@implementation ESNateOnAccount

- (const char *)protocolPlugin
{
	return "prpl-nateon";
}

- (void)configurePurpleAccount
{
	[super configurePurpleAccount];
}

- (const char *)purpleStatusIDForStatus:(AIStatus *)statusState
							arguments:(NSMutableDictionary *)arguments
{
	const char		*statusID = NULL;
	NSString		*statusName = [statusState statusName];
	NSString		*statusMessageString = [statusState statusMessageString];

	if (!statusMessageString) statusMessageString = @"";
	AILog(@"StatusName:%@", statusName);
	switch ([statusState statusType]) {
		case AIAvailableStatusType:
			if (([statusName isEqualToString:STATUS_NAME_AVAILABLE]) ||
					 ([statusMessageString caseInsensitiveCompare:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_AVAILABLE]] == NSOrderedSame))
				statusID = "O";
			break;

		case AIAwayStatusType:
			if (([statusName isEqualToString:STATUS_NAME_AWAY]) ||
					 ([statusMessageString caseInsensitiveCompare:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_AWAY]] == NSOrderedSame))
				statusID = "A";				
			else if (([statusName isEqualToString:STATUS_NAME_PHONE]) ||
					 ([statusMessageString caseInsensitiveCompare:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_PHONE]] == NSOrderedSame))
				statusID = "P";
			else if (([statusName isEqualToString:STATUS_NAME_BUSY]) ||
					 ([statusMessageString caseInsensitiveCompare:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_BUSY]] == NSOrderedSame))
				statusID = "B";
			break;
			
		case AIInvisibleStatusType:
			if (([statusName isEqualToString:STATUS_NAME_INVISIBLE]) ||
					 ([statusMessageString caseInsensitiveCompare:[[adium statusController] localizedDescriptionForCoreStatusName:STATUS_NAME_INVISIBLE]] == NSOrderedSame))
				statusID = "X";
			break;
			
		case AIOfflineStatusType:
			break;
	}
	
	//If we didn't get a purple status ID, request one from super
	if (statusID == NULL) statusID = [super purpleStatusIDForStatus:statusState arguments:arguments];
	AILog(@"statusID:%s", statusID);
	return statusID;
}

- (void)gotFilteredDisplayName:(NSAttributedString *)attributedDisplayName
{
	NSString	*friendlyName = [attributedDisplayName string];
	const char *friendlyNameUTF8String = [friendlyName UTF8String];
	if (account && purple_account_is_connected(account))
		nateon_act_id_(purple_account_get_connection(account), friendlyNameUTF8String);
	[super gotFilteredDisplayName:attributedDisplayName];
}


#pragma mark File transfer

- (BOOL)canSendFolders
{
	return NO;
}

- (void)beginSendOfFileTransfer:(ESFileTransfer *)fileTransfer
{
	[super _beginSendOfFileTransfer:fileTransfer];
}

- (void)acceptFileTransferRequest:(ESFileTransfer *)fileTransfer
{
    [super acceptFileTransferRequest:fileTransfer];    
}

- (void)rejectFileReceiveRequest:(ESFileTransfer *)fileTransfer
{
    [super rejectFileReceiveRequest:fileTransfer];    
}

- (void)cancelFileTransfer:(ESFileTransfer *)fileTransfer
{
	[super cancelFileTransfer:fileTransfer];
}


#pragma mark View Buddies By

// purple_request_choice : Adium 1.2.x just ignore this function.
// So, hide this menu.
- (NSString *)titleForAccountActionMenuLabel:(const char *)label
{	
	if (strcmp(label, _("View Buddies By...")) == 0) {
		return nil;
	}
	return [super titleForAccountActionMenuLabel:label];
}

// and add new menuitem
- (NSArray *)accountActionMenuItems
{
	NSMutableArray *menuItemArray = [[super accountActionMenuItems] mutableCopy];
	NSMenuItem			*menuItem;
	
	[menuItemArray addObject:[NSMenuItem separatorItem]];
	menuItem = [[[NSMenuItem allocWithZone:[NSMenu menuZone]] initWithTitle:@"Change Alias Format"
																	 target:self
																	 action:@selector(toggleNameAction:)
															  keyEquivalent:@""] autorelease];
	if (!menuItemArray) menuItemArray = [NSMutableArray array];
	
	[menuItemArray addObject:menuItem];
	
	return [menuItemArray autorelease];
}

- (void)toggleNameAction:(NSMenuItem *)sender
{
	int choice = purple_account_get_int(account, "view_buddies_by", NATEON_VIEW_BUDDIES_BY_SCREEN_NAME);

	if (choice == NATEON_VIEW_BUDDIES_BY_NAME_AND_SCREEN_NAME) {
		choice = NATEON_VIEW_BUDDIES_BY_NAME;
	} else {
		choice++;
	}
	[self viewBuddiesBy:choice];
}


- (void)viewBuddiesBy: (int)choice
{
	PurpleBlistNode *gnode, *cnode, *bnode;
	
	purple_account_set_int(account, "view_buddies_by", choice);
	
	for (gnode = purple_blist_get_root(); gnode; gnode = gnode->next) {
		//PurpleGroup *group = (PurpleGroup *)gnode;
		if(!PURPLE_BLIST_NODE_IS_GROUP(gnode))
			continue;
		for(cnode = gnode->child; cnode; cnode = cnode->next) {
			if(!PURPLE_BLIST_NODE_IS_CONTACT(cnode))
				continue;
			for(bnode = cnode->child; bnode; bnode = bnode->next) {
				PurpleBuddy *b;
				if(!PURPLE_BLIST_NODE_IS_BUDDY(bnode))
					continue;
				b = (PurpleBuddy *)bnode;
				if(!strcmp(purple_buddy_get_account(b)->username, account->username)) {
					[self setUserAlias: b->proto_data Choice: choice];
				}
			}
		}
	}
}


- (void)setUserAlias: (NateonUser *)user Choice: (int)choice
{
	const char *friend;
	const char *account_name;
	const char *store;
	char *alias;

	account_name = nateon_user_get_account_name(user);
	store = nateon_user_get_store_name(user);
	friend = nateon_user_get_friendly_name(user);
	
	switch (choice) {
		case NATEON_VIEW_BUDDIES_BY_NAME:
			alias = g_strdup(friend);
			break;

		case NATEON_VIEW_BUDDIES_BY_NAME_AND_ID:
			alias = g_strdup_printf("%s (%s)", friend, account_name);
			break;

		case NATEON_VIEW_BUDDIES_BY_NAME_AND_SCREEN_NAME:
			if (store)
			{
				alias = g_strdup_printf("%s (%s)", friend, store);
			}
			else
			{
				alias = g_strdup_printf("%s (%s)", friend, friend);
			}
			break;

		case NATEON_VIEW_BUDDIES_BY_SCREEN_NAME:
		default:
			if (store)
			{
				alias = g_strdup(store);
			} 
			else
			{
				alias = g_strdup(friend);
			}
			break;
	}
	
	serv_got_alias(purple_account_get_connection(account), account_name, alias);
	g_free(alias);
}
@end
/*
void nateon_act_view_buddies_by_alt(PurpleAccount *account, int choice)
{
	PurpleBlistNode *gnode, *cnode, *bnode;
	PurpleConnection *gc;
	
	gc = purple_account_get_connection(account);
	
	purple_account_set_int(account, "view_buddies_by", choice);
	
	for (gnode = purple_blist_get_root(); gnode; gnode = gnode->next) {
		//PurpleGroup *group = (PurpleGroup *)gnode;
		if(!PURPLE_BLIST_NODE_IS_GROUP(gnode))
			continue;
		for(cnode = gnode->child; cnode; cnode = cnode->next) {
			if(!PURPLE_BLIST_NODE_IS_CONTACT(cnode))
				continue;
			for(bnode = cnode->child; bnode; bnode = bnode->next) {
				PurpleBuddy *b;
				if(!PURPLE_BLIST_NODE_IS_BUDDY(bnode))
					continue;
				b = (PurpleBuddy *)bnode;
				//if(purple_buddy_get_account(b) == account) {
				if( !strcmp(purple_buddy_get_account(b)->username, account->username) ) {
					nateon_user_set_buddy_alias_alt(gc->proto_data, b->proto_data);
				}
			}
		}
	}
}

void nateon_user_set_buddy_alias_alt(NateonSession *session, NateonUser *user)
{
	PurpleAccount *account;
	PurpleConnection *gc;
	const char *friend;
	const char *account_name;
	const char *store;
	char *alias;
	BOOL choice;
	
	account = session->account;
	gc = purple_account_get_connection(account);
	
	account_name = nateon_user_get_account_name(user);
	store = nateon_user_get_store_name(user);
	friend = nateon_user_get_friendly_name(user);
	
	choice = purple_account_get_int(account, "view_buddies_by", NO);
	
	switch (choice) {
		case NO:
			alias = g_strdup(friend);
			break;
		case YES:
			alias = g_strdup(store);
			break;
		default:
			if (store)
			{
				alias = g_strdup(store);
			} 
			else
			{
				alias = g_strdup(friend);
			}
			break;
	}
	
	serv_got_alias(gc, account_name, alias);
	g_free(alias);
}
*/
