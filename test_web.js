const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

const html = fs.readFileSync("docs/index.html", "utf8");
const match = html.match(/<script>([\s\S]*?)<\/script>/);
assert.ok(match, "The browser app must contain its JavaScript");
const script = match[1];

const stored = Object.create(null);
const listeners = Object.create(null);
const nodes = Object.create(null);
function getNode(id) {
  if (!nodes[id]) {
    nodes[id] = {
      innerHTML: "",
      textContent: "",
      classList: { add() {}, remove() {} },
      addEventListener(type, callback) {
        listeners[id + ":" + type] = callback;
      }
    };
  }
  return nodes[id];
}
const context = {
  document: { getElementById: getNode },
  localStorage: {
    getItem(key) { return Object.prototype.hasOwnProperty.call(stored, key) ? stored[key] : null; },
    setItem(key, value) { stored[key] = String(value); },
    removeItem(key) { delete stored[key]; }
  },
  window: { confirm() { return true; } },
  setTimeout() { return 1; },
  clearTimeout() {}
};
vm.runInNewContext(script, context, { filename: "docs/index.html.inline.js" });

const app = getNode("app");
function click(action, attrs = {}) {
  const button = {
    getAttribute(name) {
      if (name === "data-action") return action;
      const key = name.indexOf("data-") === 0 ? name.slice(5) : name;
      return Object.prototype.hasOwnProperty.call(attrs, key) ? String(attrs[key]) : null;
    }
  };
  listeners["app:click"]({ target: { closest(selector) { return selector === "[data-action]" ? button : null; } } });
}
function submit(id, fields) {
  const elements = {};
  Object.keys(fields).forEach(key => { elements[key] = { value: String(fields[key]) }; });
  let prevented = false;
  listeners["app:submit"]({
    target: { id, elements },
    preventDefault() { prevented = true; }
  });
  assert.ok(prevented, "Form " + id + " should be handled by the app");
}

assert.match(app.innerHTML, /Cafe QuickOrder|Sign in|Student/i);
click("switch-role", { role: "admin" });
assert.match(app.innerHTML, /admin-login-form/);
submit("admin-login-form", { pin: "wrong" });
assert.match(getNode("toast").textContent, /Incorrect/i);
submit("admin-login-form", { pin: "1234" });
assert.match(app.innerHTML, /Restaurant dashboard/);

click("admin-page", { page: "menu" });
assert.match(app.innerHTML, /Sandwich/);
submit("menu-form", { id: "", name: "Paneer Roll", price: "110", description: "Test roll", emoji: "🌯" });
assert.match(app.innerHTML, /Paneer Roll/);
click("edit-item", { id: 7 });
submit("menu-form", { id: "7", name: "Cheese Roll", price: "125", description: "Updated roll", emoji: "🌯" });
assert.match(app.innerHTML, /Cheese Roll/);
assert.match(app.innerHTML, /₹125/);
click("toggle-item", { id: 7 });
assert.match(app.innerHTML, /Unavailable/);
click("toggle-item", { id: 7 });
click("delete-item", { id: 7 });
assert.doesNotMatch(app.innerHTML, /Cheese Roll/);

click("logout");
click("switch-role", { role: "student" });
click("auth-mode", { mode: "register" });
submit("register-form", { id: "STU1", name: "Test Student", password: "pw1234" });
assert.match(app.innerHTML, /Today.s menu|Today's menu/);
assert.match(app.innerHTML, /Sandwich/);
click("add-cart", { id: 1 });
assert.match(app.innerHTML, /Order basket/);
submit("checkout-form", { pickup: "Test Student" });
assert.match(app.innerHTML, /Order #1001/);
assert.match(app.innerHTML, /₹60/);
click("cancel-order", { id: 1001 });
assert.match(app.innerHTML, /Cancelled/);

const saved = JSON.parse(stored["cafeQuickOrderBrowser_v1"]);
assert.ok(saved.users.stu1, "Student registration should persist");
assert.equal(saved.orders.length, 1, "Placed order should persist");
assert.equal(saved.orders[0].active, false, "Cancellation should persist");
assert.equal(saved.orders[0].total, 60, "Bill total should be calculated correctly");
assert.ok(saved.menu.some(item => item.name === "Sandwich"), "Menu data should persist");

console.log("PASS: browser app renders, admin PIN validation works, and menu add/edit/availability/delete work");
console.log("PASS: student registration, cart, billing, order history, cancellation and persistence work");
