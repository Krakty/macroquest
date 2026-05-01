# Credential Mapping

Non-secret-revealing inventory of which credentials are needed where. The
actual keys/tokens are NOT in this file — this is a map.

## Required credentials

| Credential | Used for | Stored where |
|---|---|---|
| GitHub PAT | `gh` CLI on BEAST (creating repos, listing PRs, branch protection API) | `~/.config/gh/hosts.yml` (managed by `gh auth login`) |
| SSH key for `git@github.com` | Push/pull on BEAST | `~/.ssh/id_*` (per-user, not in repo) |
| SSH key for wintest | BEAST → wintest connections (build, status queries) | `~/.ssh/config.d/wintest` references the key path |
| SSH key for laptop (10.7.30.37) | BEAST → laptop deploy (scp) | `~/.ssh/config.d/laptop` (or similar) references the key path |

## SSH config pattern

Both wintest and laptop use the standard `~/.ssh/config.d/` include pattern.
Example structure (substitute real values):

```
# ~/.ssh/config.d/wintest
Host wintest
    HostName <wintest-ip-or-fqdn>
    User Administrator
    IdentityFile ~/.ssh/<key-name-for-wintest>
    PreferredAuthentications publickey
```

```
# ~/.ssh/config.d/laptop
Host 10.7.30.37
    User tlindell
    IdentityFile ~/.ssh/<key-name-for-laptop>
    PreferredAuthentications publickey
```

`~/.ssh/config` includes the directory:
```
Include ~/.ssh/config.d/*
```

## What goes where on each host

### BEAST (development host)
- GitHub PAT (for `gh`)
- SSH private key for github.com
- SSH private key for wintest
- SSH private key for 10.7.30.37 (laptop)

### Wintest (build server)
- SSH `authorized_keys` accepting BEAST's public key
- NO push credentials to GitHub. Wintest is pull-only. Pull is via HTTPS or
  via deploy-key SSH (read-only). Either way, no write access.

### Laptop (deploy target)
- SSH `authorized_keys` accepting BEAST's public key
- No GitHub access at all. Laptop only receives DLLs via scp.

## What's NOT in this repo

- Real SSH keys (private or public). Keys are per-machine.
- Real GitHub PAT.
- Real WiFi/network credentials.
- Anything else security-sensitive.

If you find any of the above committed accidentally, treat as a security
incident: rotate the credential immediately, then `git filter-repo` to scrub
the history, force-push (this is the ONE case where force-push is allowed
on `release/*` — protected branch rules will need temporarily lifting).

## See also

- `docs/SETUP.md` — initial setup procedure that establishes these
  credentials on a fresh dev box
- `docs/WORKFLOW.md` — the rules around when each credential is used
