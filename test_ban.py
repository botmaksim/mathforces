import urllib.request
import json
import ssl

def request(url, method="GET", data=None, headers={}):
    req = urllib.request.Request(url, method=method)
    for k, v in headers.items():
        req.add_header(k, v)
    if data:
        req.data = json.dumps(data).encode('utf-8')
        req.add_header("Content-Type", "application/json")
    try:
        ctx = ssl.create_default_context()
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        resp = urllib.request.urlopen(req, context=ctx)
        return resp.read().decode('utf-8'), resp.status
    except urllib.error.HTTPError as e:
        return e.read().decode('utf-8'), e.code

print("Logging in...")
body, code = request("http://127.0.0.1:3000/api/login/email", "POST", {"email": "superadmin@example.com", "password": "superadmin_pass"})
if code != 200:
    print("Login error:", body)
    exit(1)

j = json.loads(body)
token = j.get("token")
print("Got token length:", len(token) if token else 0)

print("\nFetching users...")
body, code = request("http://127.0.0.1:3000/api/users", "GET", headers={"Authorization": token})
if code != 200:
    print("Users error:", body)
    exit(1)

users = json.loads(body)
print("Users:", [u['username'] for u in users])

student_id = next(u['id'] for u in users if u['username'] == 'student')

print("\nBanning user id", student_id)
body, code = request("http://127.0.0.1:3000/api/users/ban", "POST", {"user_id": student_id, "is_banned": True}, headers={"Authorization": token})
print("Ban response:", code, body)

