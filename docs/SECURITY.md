# Security Policy

## Supported Versions

We release patches for security vulnerabilities for the following versions:

| Version | Supported          | Release Date   | End of Life    |
| ------- | ------------------ | -------------- | -------------- |
| 1.x.x   | :white_check_mark: | September 2023 | TBD            |
| 0.x.x   | :x:                | March 2023     | September 2023 |

Only the latest stable version receives security updates. We strongly recommend always using the latest version.

## Reporting a Vulnerability

### Contact

The DiskSense64 team takes security seriously. We appreciate your efforts to responsibly disclose your findings, and will make every effort to acknowledge your contributions.

To report a security vulnerability, please email us at:

**security@disksense64.org**

Please DO NOT create a GitHub issue for security vulnerabilities, as this would make the vulnerability public and could potentially harm users.

### What to Include in Your Report

When reporting a vulnerability, please include:

1. **Description**: A clear description of the vulnerability
2. **Impact**: The potential impact of the vulnerability
3. **Reproduction Steps**: Detailed steps to reproduce the issue
4. **Affected Versions**: Which versions are affected
5. **Workaround**: If you know of a workaround, please include it
6. **Proof of Concept**: Code or steps to demonstrate the vulnerability
7. **Environment**: Operating system, hardware, and other relevant details

### What to Expect

After you submit a vulnerability report:

1. **Acknowledgment**: You will receive an acknowledgment within 48 hours
2. **Confirmation**: We will confirm the vulnerability within 5 business days
3. **Assessment**: We will assess the severity and impact
4. **Fix Development**: We will develop a fix if the vulnerability is confirmed
5. **Coordination**: We may coordinate with you on the fix and disclosure
6. **Release**: We will release a patch as soon as possible
7. **Disclosure**: We will publicly disclose the vulnerability after the patch is released

### Timeline

Our target timeline for vulnerability handling:

- **Initial Response**: Within 48 hours
- **Confirmation**: Within 5 business days
- **Fix Development**: Depends on complexity (typically 1-4 weeks)
- **Patch Release**: As soon as fix is ready and tested
- **Public Disclosure**: 30-90 days after patch release

### Severity Classification

We classify security vulnerabilities into the following categories:

#### Critical (CVSS 9.0-10.0)
- Remote code execution
- Privilege escalation to administrator/system
- Complete data compromise
- Complete service disruption

#### High (CVSS 7.0-8.9)
- Local privilege escalation
- Significant data exposure
- Service disruption affecting multiple users
- Bypass of security controls

#### Medium (CVSS 4.0-6.9)
- Limited data exposure
- Denial of service affecting single user
- Information disclosure without privilege escalation

#### Low (CVSS 0.1-3.9)
- Minor information disclosure
- Minor denial of service
- Issues requiring unlikely conditions to exploit

### Coordinated Disclosure

We believe in coordinated disclosure and will work with you to:

1. Confirm the vulnerability
2. Develop and test a fix
3. Release the patch
4. Publicly disclose the vulnerability

We request that you:

- Give us reasonable time to fix the issue before public disclosure
- Not disclose the vulnerability to third parties without our consent
- Coordinate with us on the disclosure timeline

### Recognition

We believe in recognizing security researchers who help improve DiskSense64's security. With your permission, we will:

- Acknowledge your contribution in our release notes
- Add you to our security hall of fame
- Provide a CVE identifier if applicable

### Bounty Program

Currently, DiskSense64 does not offer a formal bug bounty program. However, we greatly appreciate security research and may offer recognition or rewards for significant contributions.

For critical vulnerabilities that affect user data or system security, we may consider offering a reward. Please indicate in your report if you would like to be considered for a reward.

### Security Features

DiskSense64 includes several built-in security features:

#### File System Security
- **Access Control**: Respects file system permissions
- **Privilege Dropping**: Drops elevated privileges when not needed
- **Secure Deletion**: Securely deletes sensitive files when requested
- **Integrity Checking**: Verifies file integrity during operations

#### Data Protection
- **Encryption**: Encrypts sensitive configuration data
- **Hash Verification**: Uses cryptographic hashes to verify file integrity
- **Secure Communication**: Uses secure protocols for network operations
- **Memory Protection**: Protects sensitive data in memory

#### Privacy
- **Local Processing**: All processing happens locally
- **No Data Collection**: Does not collect or transmit user data
- **Transparent Operations**: Clearly indicates all file operations
- **User Consent**: Requires explicit user consent for destructive operations

#### Secure Coding Practices
- **Input Validation**: Validates all inputs
- **Buffer Overflow Protection**: Uses safe string handling
- **Memory Safety**: Uses RAII and smart pointers
- **Error Handling**: Proper error handling and logging

### Known Security Limitations

While we strive to make DiskSense64 as secure as possible, there are some inherent limitations:

1. **File System Permissions**: DiskSense64 can only operate within the permissions granted to the user
2. **Operating System Security**: Security ultimately depends on the underlying OS security model
3. **Third-Party Dependencies**: Security of third-party libraries is outside our direct control
4. **Physical Security**: Cannot protect against physical access to storage devices
5. **Network Security**: Network operations depend on network security infrastructure

### Security Testing

We regularly perform security testing including:

#### Static Analysis
- Code review for security issues
- Automated static analysis tools
- Manual security auditing

#### Dynamic Analysis
- Penetration testing
- Fuzz testing
- Runtime analysis

#### Third-Party Audits
- Independent security reviews
- Code audits by security experts
- Compliance verification

### Incident Response

In the event of a security incident:

1. **Containment**: Immediately contain the incident
2. **Investigation**: Investigate the scope and impact
3. **Communication**: Communicate with affected users
4. **Remediation**: Apply fixes and patches
5. **Post-Incident Review**: Review and improve processes

### Compliance

DiskSense64 complies with the following security standards:

- **OWASP Top 10**: Addresses the OWASP Top 10 security risks
- **CWE**: Addresses common weakness enumeration items
- **NIST**: Follows NIST cybersecurity framework guidelines
- **ISO 27001**: Follows information security management principles

### Security Resources

For more information about DiskSense64 security:

- **Security Blog**: https://disksense64.org/security
- **Advisory Archive**: https://disksense64.org/security/advisories
- **CVE List**: https://cve.mitre.org/cgi-bin/cvekey.cgi?keyword=disksense64
- **Security Mailing List**: security-announce@disksense64.org

### Questions

If you have any questions about this security policy, please contact:

**security@disksense64.org**

We appreciate your help in keeping DiskSense64 secure for everyone.