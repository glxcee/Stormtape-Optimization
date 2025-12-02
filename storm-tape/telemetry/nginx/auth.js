async function authenticate_and_authorize(r) {
  r.log("OPA REQ", r);

  r.log("now sending to opa");
  const opa_res = await r.subrequest("/_opa", {
    method: r.variables["request_method"],
    body: r.requestBuffer,
  });
  r.log("opa answered");

  r.log("now sending to storm-tape");
  const storm_res = await r.subrequest(
    "/_storm-tape" + r.variables["request_uri"].replace(/\/$/, ""),
    {
      method: r.variables["request_method"],
      body: r.requestBuffer,
    }
  );
  r.log("storm answered");

  r.return(storm_res.status, storm_res.responseText);
}

export default { authenticate_and_authorize };
